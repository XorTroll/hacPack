#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include "pfs0.h"
#include "sha.h"

#include "types.h"

#define MAX_FS_ENTRIES 0x10 //If ever needed, this constant and the size of stringtable can be increased.

int pfs0_build(filepath_t *in_dirpath, filepath_t *out_pfs0_filepath, uint64_t *out_pfs0_size)
{
    DIR *dir = NULL;
    struct dirent *cur_dirent = NULL;
    struct stat objstats;
    FILE *fout = NULL, *fin = NULL;
    int ret = 0;
    uint32_t tmplen = 0;
    uint32_t pos;
    uint8_t *tmpbuf;

    uint32_t objcount = 0;
    uint32_t stringtable_offset = 0;
    uint64_t filedata_reloffset = 0;

    pfs0_header_t header;
    pfs0_file_entry_t fsentries[MAX_FS_ENTRIES];
    pfs0_file_entry_t *fsentry;

    char objpath[257];

    char stringtable[0x200];

    memset(&header, 0, sizeof(header));
    memset(fsentries, 0, sizeof(fsentries));
    memset(stringtable, 0, sizeof(stringtable));

    filepath_t in_dirpath_cpy;
    filepath_init(&in_dirpath_cpy);
    filepath_copy(&in_dirpath_cpy, in_dirpath);
    if (strcmp(&in_dirpath_cpy.char_path[strlen(in_dirpath_cpy.char_path) - 1], OS_PATH_SEPARATOR) != 0)
        filepath_append(&in_dirpath_cpy, "");

    dir = opendir(in_dirpath_cpy.char_path);
    if (dir == NULL)
    {
        printf("Failed to open %s.\n", in_dirpath_cpy.char_path);
        return 1;
    }

    fout = os_fopen(out_pfs0_filepath->os_path, OS_MODE_WRITE);
    if (fout == NULL)
    {
        printf("Failed to open PFS0 filepath.\n");
        closedir(dir);
        return 1;
    }

    while ((cur_dirent = readdir(dir)))
    {
        if (strcmp(cur_dirent->d_name, ".") == 0 || strcmp(cur_dirent->d_name, "..") == 0)
            continue;

        memset(objpath, 0, sizeof(objpath));
        snprintf(objpath, sizeof(objpath) - 1, "%s%s", in_dirpath_cpy.char_path, cur_dirent->d_name);

        if (stat(objpath, &objstats) == -1)
        {
            printf("Failed to stat: %s\n", objpath);
            exit(EXIT_FAILURE);
        }

        if ((objstats.st_mode & S_IFMT) == S_IFDIR) //directory
        {
            printf("Directories aren't supported, skipping... (%s)\n", objpath);
        }
        else if ((objstats.st_mode & S_IFMT) == S_IFREG) //file
        {
            if (objcount >= MAX_FS_ENTRIES)
            {
                printf("Maximum fs object count already reached.\n");
                exit(EXIT_FAILURE);
            }

            fsentry = &fsentries[objcount];

            fsentry->offset = filedata_reloffset;
            fsentry->size = objstats.st_size;
            filedata_reloffset += fsentry->size;
            fsentry->string_table_offset = stringtable_offset;

            tmplen = strlen(cur_dirent->d_name) + 1;
            if (stringtable_offset + tmplen > sizeof(stringtable))
            {
                printf("Max size of stringtable reached.\n");
                exit(EXIT_FAILURE);
            }

            strncpy(&stringtable[stringtable_offset], cur_dirent->d_name, sizeof(stringtable) - stringtable_offset);
            stringtable_offset += tmplen;

            objcount++;
        }
        else
        {
            printf("Invalid FS object type.\n");
            exit(EXIT_FAILURE);
        }
    }

    closedir(dir);

    if (ret == 0)
    {
        stringtable_offset = (stringtable_offset + 0x1f) & ~0x1f;

        header.magic = le_word(0x30534650);
        header.num_files = le_word(objcount);
        header.string_table_size = le_word(stringtable_offset);

        fwrite(&header, 1, sizeof(header), fout);
        fwrite(fsentries, 1, sizeof(pfs0_file_entry_t) * objcount, fout);
        fwrite(stringtable, 1, stringtable_offset, fout);

        stringtable_offset = 0;

        for (pos = 0; pos < objcount; pos++)
        {
            tmplen = strlen(&stringtable[stringtable_offset]);
            if (tmplen == 0)
            {
                printf("Empty string entry found in stringtable.\n");
                ret = 5;
                break;
            }
            tmplen++;

            if (stringtable_offset + tmplen > sizeof(stringtable))
            {
                printf("Max size of stringtable reached during stringtable entry reading.\n");
                ret = 4;
                break;
            }

            memset(objpath, 0, sizeof(objpath));
            snprintf(objpath, sizeof(objpath) - 1, "%s%s", in_dirpath_cpy.char_path, &stringtable[stringtable_offset]);
            stringtable_offset += tmplen;

            fin = fopen(objpath, "rb");
            if (fin == NULL)
            {
                printf("Failed to open filepath for filedata.\n");
                ret = 1;
                break;
            }

            tmpbuf = malloc(fsentries[pos].size);
            if (tmpbuf == NULL)
            {
                printf("Failed to allocate filedata.\n");
                ret = 6;
                fclose(fin);
                break;
            }

            printf("Writing %s to %s\n", objpath, out_pfs0_filepath->char_path);

            tmplen = fread(tmpbuf, 1, fsentries[pos].size, fin);
            fclose(fin);

            fwrite(tmpbuf, 1, fsentries[pos].size, fout);
            free(tmpbuf);
        }
    }

    *out_pfs0_size = (uint64_t)ftello64(fout);

    fclose(fout);

    return ret;
}

void pfs0_create_hashtable(filepath_t *pfs0_path, filepath_t *pfs0_hashtable_path, uint64_t *out_hashtable_size, uint64_t *out_pfs0_offset)
{
    FILE *src_file;
    FILE *dst_file;
    uint64_t hash_block_size = PFS0_HASH_BLOCK_SIZE;

    // Open files
    src_file = os_fopen(pfs0_path->os_path, OS_MODE_READ);
    if (src_file == NULL)
    {
        fprintf(stderr, "Unable to open: %s", pfs0_path->char_path);
        exit(EXIT_FAILURE);
    }
    dst_file = os_fopen(pfs0_hashtable_path->os_path, OS_MODE_WRITE);
    if (dst_file == NULL)
    {
        fprintf(stderr, "Unable to open: %s", pfs0_hashtable_path->char_path);
        exit(EXIT_FAILURE);
    }

    uint64_t read_size = hash_block_size;
    uint64_t src_file_size;
    unsigned char *hash = (unsigned char *)malloc(0x20);

    // Get source file size
    fseeko64(src_file, 0, SEEK_END);
    src_file_size = ftello64(src_file);

    unsigned char *buf = calloc(1, read_size);
    fseeko64(src_file, 0, SEEK_SET);
    fseeko64(dst_file, 0, SEEK_SET);

    if (buf == NULL)
    {
        fprintf(stderr, "Failed to allocate file-read buffer!\n");
        exit(EXIT_FAILURE);
    }
    uint64_t ofs = 0;

    while (ofs < src_file_size)
    {
        sha_ctx_t *sha_ctx = new_sha_ctx(HASH_TYPE_SHA256, 0);
        if (ofs + read_size >= src_file_size)
            read_size = src_file_size - ofs;
        if (fread(buf, 1, read_size, src_file) != read_size)
        {
            fprintf(stderr, "Failed to read file: %s!\n", pfs0_path->char_path);
            exit(EXIT_FAILURE);
        }
        sha_update(sha_ctx, buf, read_size);
        sha_get_hash(sha_ctx, hash);
        fwrite(hash, 0x20, 1, dst_file);
        free_sha_ctx(sha_ctx);
        ofs += read_size;
    }

    uint64_t curr_offset = (uint64_t)ftello64(dst_file);
    *out_hashtable_size = curr_offset;

    // Write Padding
    uint64_t pfs0_paddingsize = PFS0_PADDING_SIZE;
    uint64_t padding_size = pfs0_paddingsize - (curr_offset % pfs0_paddingsize);
    if (curr_offset % hash_block_size != 0)
    {
        memset(buf, 0, hash_block_size);
        fwrite(buf, 1, padding_size, dst_file);
    }
    *out_pfs0_offset = (uint64_t)ftello64(dst_file);

    free(buf);
    fclose(src_file);
    fclose(dst_file);
}

void pfs0_calculate_master_hash(filepath_t *pfs0_hashtable_filepath, uint64_t hash_table_size, uint8_t *out_master_hash)
{
    FILE *pfs0_hashtable_file;
    pfs0_hashtable_file = os_fopen(pfs0_hashtable_filepath->os_path, OS_MODE_READ);
    if (pfs0_hashtable_file == NULL)
    {
        fprintf(stderr, "Unable to open: %s", pfs0_hashtable_filepath->char_path);
        exit(EXIT_FAILURE);
    }

    // Calculate hash
    unsigned char *buf = (unsigned char *)malloc(hash_table_size);
    sha_ctx_t *sha_ctx = new_sha_ctx(HASH_TYPE_SHA256, 0);
    uint64_t read_size = 0x61A8000; // 100 MB buffer.
    uint64_t ofs = 0;
    while (ofs < hash_table_size)
    {
        if (ofs + read_size >= hash_table_size)
            read_size = hash_table_size - ofs;
        if (fread(buf, 1, hash_table_size, pfs0_hashtable_file) != hash_table_size)
        {
            fprintf(stderr, "Failed to read file: %s!\n", pfs0_hashtable_filepath->char_path);
            exit(EXIT_FAILURE);
        }
        sha_update(sha_ctx, buf, hash_table_size);
        ofs += read_size;
    }
    sha_get_hash(sha_ctx, (unsigned char *)out_master_hash);

    free_sha_ctx(sha_ctx);
    free(buf);
    fclose(pfs0_hashtable_file);
}