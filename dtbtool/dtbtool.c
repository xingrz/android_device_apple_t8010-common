/*
 * Copyright (C) 2020 The MoKee Open Source Project
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#define INPUT_PATTERN  "%shx/hx-h9p-%s.dtb"
#define MAX_DTB_SIZE   1 * 1024 * 1024

const uint8_t header[] = {'C', 'o', 'w', 's'};
const uint8_t footer[] = {0, 0, 0, 0, 0};

static char *input_dir;
static char *output_file;
static char *devices;

static void print_help() {
    printf("dtbTool [options] -o <output file> <input DTB path>\n");
    printf("  options:\n");
    printf("  --output-file/-o     output file\n");
    printf("  --devices/-d         list of devices\n");
    printf("  --help/-h            this help screen\n");
}

static int parse_commandline(int argc, char *const argv[]) {
    int c;

    struct option long_options[] = {
        {"output-file", 1, 0, 'o'},
        {"dtc-path",    1, 0, 'p'},
        {"page-size",   1, 0, 's'},
        {"devices",     1, 0, 'd'},
        {"help",        0, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "-o:d:p:s:h", long_options, NULL))
           != -1) {
        switch (c) {
        case 1:
            if (!input_dir)
                input_dir = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        case 'd':
            devices = optarg;
            break;
        case 'p':
        case 's':
            break;
        case 'h':
        default:
            return -1;
        }
    }

    if (!output_file) {
        printf("Output file must be specified\n");
        return -1;
    }

    if (!input_dir)
        input_dir = "./";

    return 0;
}

static void tolowercase(const char *input, char *output) {
    for (int i = 0; input[i]; i++) {
        output[i] = tolower(input[i]);
    }
}

static void tobigendian(const int value, uint8_t *output) {
    output[0] = (value >> 24) & 0xff;
    output[1] = (value >> 16) & 0xff;
    output[2] = (value >> 8) & 0xff;
    output[3] = value & 0xff;
}

int main(int argc, char **argv) {
    char name[20] = {0};
    char path[200] = {0};
    char *dev;
    FILE *input;
    FILE *output;
    uint8_t size[4];
    uint8_t blob[MAX_DTB_SIZE];
    size_t read, wrote;

    printf("dtbTool for Apple SoC\n");

    if (parse_commandline(argc, argv) != 0) {
        print_help();
        return -1;
    }

    printf("Input directory: '%s'\n", input_dir);
    printf("Output file: '%s'\n", output_file);

    output = fopen(output_file, "wb");
    if (output == NULL) {
        printf("! Failed to open: %s\n", output_file);
        return -1;
    }

    wrote = fwrite(header, sizeof(header), 1, output);
    if (wrote <= 0) {
        printf("! Failed to write HEADER: %s\n", output_file);
        goto err;
    }

    dev = strtok(devices, ",");
    while (dev != NULL) {
        if (strlen(dev) > sizeof(name)) {
            printf("! Device name too long: %s\n", dev);
            goto err;
        }

        printf("* Combining DTB for device: %s\n", dev);

        tolowercase(dev, name);
        sprintf(path, INPUT_PATTERN, input_dir, name);
        if (access(path, R_OK) != 0) {
            printf("! Path not found: %s\n", path);
            goto err;
        }

        input = fopen(path, "rb");
        if (input == NULL) {
            printf("! Failed to open: %s\n", path);
            goto err;
        }

        read = fread(blob, 1, MAX_DTB_SIZE, input);
        fclose(input);
        if (read <= 0) {
            printf("! Failed to read: %s\n", path);
            goto err;
        }

        strcpy(name, dev);
        wrote = fwrite(name, 1, strlen(name) + 1, output);
        if (wrote <= 0) {
            printf("! Failed to write NAME: %s\n", output_file);
            goto err;
        }

        tobigendian(read, size);
        wrote = fwrite(size, 1, sizeof(size), output);
        if (wrote <= 0) {
            printf("! Failed to write SIZE: %s\n", output_file);
            goto err;
        }

        wrote = fwrite(blob, 1, read, output);
        if (wrote <= 0) {
            printf("! Failed to write BLOB: %s\n", output_file);
            goto err;
        }

        dev = strtok(NULL, ",");
    }

    wrote = fwrite(footer, sizeof(footer), 1, output);
    if (wrote <= 0) {
        printf("! Failed to write FOOTER: %s\n", output_file);
        goto err;
    }

    return 0;
err:
    fclose(output);
    unlink(output_file);
    return 1;
}
