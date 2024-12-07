#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "fat32.h"
#include "commands.h"
#include "output.h"

/* Show usage help */
void usage(char *executable)
{
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "\t%s -h | --help for help\n", executable);
    fprintf(stdout, "\t%s ls <fat32-img> - List files from the FAT32 image\n", executable);
    fprintf(stdout, "\t%s cp <path> <dest> <fat32-img> - Copy files from the image path to local dest.\n", executable);
    fprintf(stdout, "\t%s mv <path> <dest> <fat32-img> - Move files from the path to the FAT32 path\n", executable);
    fprintf(stdout, "\t%s rm <path> <file> <fat32-img> - Remove files from the path to the FAT32 path\n", executable);
    fprintf(stdout, "\n");
    fprintf(stdout, "\tfat32-img needs to be a valid Fat32.\n\n");
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, getenv("LANG"));

    if (argc <= 1) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    if (argc < 3 || argc > 4) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(argv[argc - 1], "rb+");
    if (!fp) {
        fprintf(stderr, "Could not open file %s\n", argv[argc - 1]);
        exit(EXIT_FAILURE);
    }

    struct fat_bpb bpb;
    rfat(fp, &bpb);
    char *command = argv[1];

    if (strcmp(command, "ls") == 0) {
        struct fat_dir *dirs = ls(fp, &bpb);
        show_files(dirs);
    } else if (strcmp(command, "cp") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s cp <path> <dest> <fat32-img>\n", argv[0]);
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        cp(fp, argv[2], argv[3], &bpb);
    } else if (strcmp(command, "mv") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s mv <path> <dest> <fat32-img>\n", argv[0]);
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        mv(fp, argv[2], argv[3], &bpb);
    } else if (strcmp(command, "rm") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s rm <path> <file> <fat32-img>\n", argv[0]);
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        rm(fp, argv[2], &bpb);
    } else if (strcmp(command, "cat") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s cat <path> <fat32-img>\n", argv[0]);
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        cat(fp, argv[2], &bpb);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    fclose(fp);
    return EXIT_SUCCESS;
}