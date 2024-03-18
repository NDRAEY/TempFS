//
// Created by ndraey on 18.3.2024.
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "emu/disk.h"
#include "tempfs.h"

void util_create_empty_disk(int argc, char **argv) {
    if(argc < 3) {
        printf("Usage: %s %s filename size_in_bytes\n", argv[-1], argv[0]);
        return;
    }

    char* endptr;

    char* filename = argv[1];
    long size = strtol(argv[2], &endptr, 10);

    if(endptr == argv[2]) {
        printf("Invalid size: %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }


    FILE* file = fopen(filename, "wb");

    fseek(file, size - 1, SEEK_SET);

    fputc(0, file);

    fclose(file);
}

void util_create_disk(int argc, char **argv) {
    if(argc < 3) {
        printf("Usage: %s %s filename size_in_bytes\n", argv[-1], argv[0]);
        return;
    }

    char* filename = argv[1];

    FILE* file = fopen(filename, "wb");

    if(file == NULL) {
        printf("Could not open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fclose(file);


    util_create_empty_disk(argc, argv);


    dpm_init(filename);
    fs_tempfs_detect('T');
    fs_tempfs_format('T');

    printf("PIMNIK98! WHY ARE YOU FUCKING HARDCODED THE `disk.img` INTO WHOLE PROJECT I CAN'T CREATE A DISK AT THE ANOTHER FILE LOCATION\n");
}

struct command {
    char* name;
    void (*func)(int argc, char** argv);
};

static const struct command commands[] = {
        {"disk", util_create_empty_disk},
        {"create", util_create_disk},
};

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    char *command = argv[1];
    int command_count = sizeof(commands) / sizeof(struct command);

    bool found = false;
    for(int i = 0; i < command_count; i++) {
        if(strcmp(commands[i].name, command) == 0) {
            commands[i].func(argc - 1, argv + 1);
            found = true;
            break;
        }
    }

    if(!found) {
        fprintf(stderr, "Unknown command: %s\n", command);
    }

    return 0;
}