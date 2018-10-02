#include "ivfc.h"
#include "sha.h"
#include "string.h"

void ivfc_create_level(filepath_t *dst_level_file, filepath_t *src_level_file, uint64_t *out_size)
{
    FILE *src_file;
    FILE *dst_file;
    uint64_t hash_block_size = IVFC_HASH_BLOCK_SIZE;

    // Open files
    src_file = os_fopen(src_level_file->os_path, OS_MODE_READ);
    if (src_file == NULL)
    {
        fprintf(stderr, "Unable to open: %s", src_level_file->char_path);
        exit(EXIT_FAILURE);
    }
    dst_file = os_fopen(dst_level_file->os_path, OS_MODE_WRITE);
    if (dst_file == NULL)
    {
        fprintf(stderr, "Unable to open: %s", dst_level_file->char_path);
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
            fprintf(stderr, "Failed to read file!\n");
            exit(EXIT_FAILURE);
        }
        sha_update(sha_ctx, buf, read_size);
        sha_get_hash(sha_ctx, hash);
        fwrite(hash, 0x20, 1, dst_file);
        free_sha_ctx(sha_ctx);
        ofs += read_size;
    }

    uint64_t curr_offset = (uint64_t)ftello64(dst_file);
    uint64_t padding_size = (((curr_offset / hash_block_size) + 1) * hash_block_size) - curr_offset;
    if (curr_offset % hash_block_size != 0)
    {
        memset(buf, 0, hash_block_size);
        fwrite(buf, 1, padding_size, dst_file);
    }

    *out_size = (uint64_t)ftello64(dst_file);

    free(buf);
    fclose(src_file);
    fclose(dst_file);
}

void ivfc_calculate_master_hash(filepath_t *ivfc_level1_filepath, uint8_t *out_master_hash)
{
    uint64_t size;
    FILE *ivfc_level1_file;
    ivfc_level1_file = os_fopen(ivfc_level1_filepath->os_path, OS_MODE_READ);
    if (ivfc_level1_file == NULL)
    {
        fprintf(stderr, "Unable to open: %s", ivfc_level1_filepath->char_path);
        exit(EXIT_FAILURE);
    }

    // Get file size
    fseeko64(ivfc_level1_file, 0, SEEK_END);
    size = (uint64_t)ftello64(ivfc_level1_file);
    fseeko64(ivfc_level1_file, 0, SEEK_SET);
    
    // Calculate hash
    unsigned char *buf = (unsigned char*)malloc(size);
    sha_ctx_t *sha_ctx = new_sha_ctx(HASH_TYPE_SHA256, 0);
    if (fread(buf, 1, size, ivfc_level1_file) != size)
    {
        fprintf(stderr, "Failed to read file: %s!\n", ivfc_level1_filepath->char_path);
        exit(EXIT_FAILURE);
    }
    sha_update(sha_ctx, buf, size);
    sha_get_hash(sha_ctx, (unsigned char*)out_master_hash);

    free_sha_ctx(sha_ctx);
    free(buf);
    fclose(ivfc_level1_file);
}