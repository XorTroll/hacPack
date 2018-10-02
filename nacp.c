#include <stdlib.h>
#include <stdio.h>
#include "nacp.h"
#include "filepath.h"

void nacp_process(hp_settings_t *settings)
{
    filepath_t nacp_filepath;
    filepath_init(&nacp_filepath);
    filepath_copy(&nacp_filepath, &settings->romfs_dir);
    filepath_append(&nacp_filepath, "control.nacp");

    FILE *fl;
    fl = os_fopen(nacp_filepath.os_path, OS_MODE_READ);
    if (fl == NULL)
    {
        fprintf(stderr, "Failed to open %s!\n", nacp_filepath.char_path);
        exit(EXIT_FAILURE);
    }

    int c = 0;;
    // Check Title Name
    printf("Validating Title Name\n");
    c = fgetc(fl);
    if (c == 0)
    {
        fprintf(stderr, "Error: Invalid Title Name in control.nacp\n");
        exit(EXIT_FAILURE);
    }

    // Check Author
    printf("Validating Author\n");
    fseeko64(fl, 0x200, SEEK_SET);
    c = fgetc(fl);
    if (c == 0)
    {
        fprintf(stderr, "Error: Invalid Author in control.nacp\n");
        exit(EXIT_FAILURE);
    }

    // Read TitleID
    uint64_t title_id;
    printf("Getting TitleID\n");
    fseeko64(fl, 0x3038, SEEK_SET);
    if (fread(&title_id, 1, 0x8, fl) != 0x8)
    {
        fprintf(stderr, "Failed to read file %s\n", nacp_filepath.char_path);
        exit(EXIT_FAILURE);
    }

    printf("Validating TitleID\n");
    // Make sure that tids are the same
    if (title_id != settings->title_id)
    {
        fprintf(stderr, "Error: TitleID in control.nacp is different from entered TitleID: %016" PRIx64 " %016" PRIx64 "\n"
                        , title_id, settings->title_id);
        exit(EXIT_FAILURE);
    }

    fclose(fl);
}