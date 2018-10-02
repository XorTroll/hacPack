#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cnmt.h"
#include "filepath.h"
#include "utils.h"
#include "sha.h"

void cnmt_create_application(filepath_t *cnmt_filepath, hp_settings_t *settings)
{
    cnmt_ctx_t cnmt_ctx;
    cnmt_extended_application_header_t cnmt_ext_header;
    memset(&cnmt_ctx, 0, sizeof(cnmt_ctx));
    memset(&cnmt_ext_header, 0, sizeof(cnmt_ext_header));

    printf("Setting content records\n");
    if (settings->programnca.valid == VALIDITY_VALID)
    {
        cnmt_set_content_record(&settings->programnca, &cnmt_ctx.content_records[cnmt_ctx.content_records_count]);
        cnmt_ctx.content_records[cnmt_ctx.content_records_count].type = 0x1; // Program
        cnmt_ctx.content_records_count += 1;
    }
    if (settings->datanca.valid == VALIDITY_VALID)
    {
        cnmt_set_content_record(&settings->datanca, &cnmt_ctx.content_records[cnmt_ctx.content_records_count]);
        cnmt_ctx.content_records[cnmt_ctx.content_records_count].type = 0x2; // Data
        cnmt_ctx.content_records_count += 1;
    }
    if (settings->controlnca.valid == VALIDITY_VALID)
    {
        cnmt_set_content_record(&settings->controlnca, &cnmt_ctx.content_records[cnmt_ctx.content_records_count]);
        cnmt_ctx.content_records[cnmt_ctx.content_records_count].type = 0x3; // Control
        cnmt_ctx.content_records_count += 1;
    }
    if (settings->htmldocnca.valid == VALIDITY_VALID)
    {
        cnmt_set_content_record(&settings->htmldocnca, &cnmt_ctx.content_records[cnmt_ctx.content_records_count]);
        cnmt_ctx.content_records[cnmt_ctx.content_records_count].type = 0x4; // Offline-Manual html
        cnmt_ctx.content_records_count += 1;
    }
    if (settings->legalnca.valid == VALIDITY_VALID)
    {
        cnmt_set_content_record(&settings->legalnca, &cnmt_ctx.content_records[cnmt_ctx.content_records_count]);
        cnmt_ctx.content_records[cnmt_ctx.content_records_count].type = 0x5; // Legal html
        cnmt_ctx.content_records_count += 1;
    }

    // Common values
    cnmt_ctx.header.type = 0x80;
    cnmt_ctx.header.title_id = settings->title_id;
    cnmt_ctx.header.extended_header_size = 0x10;
    cnmt_ctx.header.content_entry_count = cnmt_ctx.content_records_count;
    cnmt_ext_header.patch_title_id = cnmt_ctx.header.title_id + 0x800;

    printf("Writing metadata header\n");
    FILE *cnmt_file;
    cnmt_file = os_fopen(cnmt_filepath->os_path, OS_MODE_WRITE);

    if (cnmt_file != NULL)
    {
        fwrite(&cnmt_ctx.header, 1, sizeof(cnmt_header_t), cnmt_file);
        fwrite(&cnmt_ext_header, 1, sizeof(cnmt_extended_application_header_t), cnmt_file);
    }
    else
    {
        fprintf(stderr, "Failed to create %s!\n", cnmt_filepath->char_path);
        exit(EXIT_FAILURE);
    }

    // Write content records
    printf("Writing content records\n");
    for (int i=0; i < cnmt_ctx.content_records_count; i++)
        fwrite(&cnmt_ctx.content_records[i], sizeof(cnmt_content_record_t), 1, cnmt_file);
    fwrite(settings->digest, 1, 0x20, cnmt_file);

    fclose(cnmt_file);
}

void cnmt_create_addon(filepath_t *cnmt_filepath, hp_settings_t *settings)
{
    cnmt_ctx_t cnmt_ctx;
    cnmt_extended_addon_header_t cnmt_ext_header;
    memset(&cnmt_ctx, 0, sizeof(cnmt_ctx));
    memset(&cnmt_ext_header, 0, sizeof(cnmt_ext_header));

    printf("Setting content records\n");
    if (settings->publicdatanca.valid == VALIDITY_VALID)
    {
        cnmt_set_content_record(&settings->publicdatanca, &cnmt_ctx.content_records[cnmt_ctx.content_records_count]);
        cnmt_ctx.content_records[cnmt_ctx.content_records_count].type = 0x2; // Data
        cnmt_ctx.content_records_count += 1;
    }

    // Common values
    cnmt_ctx.header.type = 0x82;
    cnmt_ctx.header.title_id = settings->title_id;
    cnmt_ctx.header.extended_header_size = 0x10;
    cnmt_ctx.header.content_entry_count = cnmt_ctx.content_records_count;
    cnmt_ext_header.application_title_id = (cnmt_ctx.header.title_id - 0x1000) & 0xFFFFFFFFFFFFF000;

    printf("Writing metadata header\n");
    FILE *cnmt_file;
    cnmt_file = os_fopen(cnmt_filepath->os_path, OS_MODE_WRITE);

    if (cnmt_file != NULL)
    {
        fwrite(&cnmt_ctx.header, 1, sizeof(cnmt_header_t), cnmt_file);
        fwrite(&cnmt_ext_header, 1, sizeof(cnmt_extended_addon_header_t), cnmt_file);
    }
    else
    {
        fprintf(stderr, "Failed to create %s!\n", cnmt_filepath->char_path);
        exit(EXIT_FAILURE);
    }

    // Write content records
    printf("Writing content records\n");
    for (int i=0; i < cnmt_ctx.content_records_count; i++)
        fwrite(&cnmt_ctx.content_records[i], sizeof(cnmt_content_record_t), 1, cnmt_file);
    fwrite(settings->digest, 1, 0x20, cnmt_file);

    fclose(cnmt_file);
}

void cnmt_set_content_record(filepath_t *nca_path, cnmt_content_record_t *content_record)
{
    FILE *nca_file;
    nca_file = os_fopen(nca_path->os_path, OS_MODE_READ);
    if (nca_file == NULL)
    {
        fprintf(stderr, "Unable to open: %s", nca_path->char_path);
        exit(EXIT_FAILURE);
    }

    // Calculate nca size
    uint64_t ncasize;
    fseeko64(nca_file, 0, SEEK_END);
    ncasize = ftello64(nca_file);
    fseeko64(nca_file, 0, SEEK_SET);

    // Calculate hash
    sha_ctx_t *sha_ctx = new_sha_ctx(HASH_TYPE_SHA256, 0);
    uint64_t read_size = 0x61A8000; // 100 MB buffer.
    unsigned char *buf = (unsigned char *)malloc(read_size);
    uint64_t ofs = 0;
    while (ofs < ncasize)
    {
        if (ofs + read_size >= ncasize)
            read_size = ncasize - ofs;
        if (fread(buf, 1, read_size, nca_file) != read_size)
        {
            fprintf(stderr, "Failed to read file: %s!\n", nca_path->char_path);
            exit(EXIT_FAILURE);
        }
        sha_update(sha_ctx, buf, read_size);
        ofs += read_size;
    }
    sha_get_hash(sha_ctx, content_record->hash);

    // ncaid = first 16 bytes of hash
    memcpy(content_record->ncaid, content_record->hash, 0x10);
    memcpy(content_record->size, &ncasize, 0x6);

    free_sha_ctx(sha_ctx);
    free(buf);
    fclose(nca_file);
}