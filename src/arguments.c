#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include "arguments.h"

char* bin_file;             // -i <file>
int rom_size     = 0x8000;  // -s <size>
int rom_offset   = 0x8000;  // -o <offset>

void print_usage(const char* app_name) {
    printf("Usage: %s [options]\n", app_name);
    printf("Options:\n");
    printf("  -h                Display this message.\n");
    printf("  -i <file>         The binary file to execute.\n");
    printf("  -R <size>         Set the ROM size. Default: 0x8000\n");
    printf("  -O <offset>       Set the ROM offset. Default: 0x8000\n");
}

// return 1 if should abort, 0 otherwise
int validate_args(void) {
    if (!bin_file) {
        fprintf(stderr, "No binary file provided.\n");
        return 1;
    }

    return 0;
}

// return 1 if should abort, 0 otherwise
int arguments_read(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hi:s:o:")) != -1) {
        switch (opt) {
            case 'i':
                bin_file = optarg;
                break;

            case 'R':
                rom_size = atoi(optarg);
                break;

            case 'O':
                rom_offset = atoi(optarg);
                break;

            case 'h':
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    return validate_args();
}

void arguments_free(void) {
}
