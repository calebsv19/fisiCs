// SPDX-License-Identifier: Apache-2.0

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

char* readFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Error: could not allocate memory for file %s\n", filename);
        exit(1);
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);

#if DEBUG_PRINT_SOURCE
    printf("DEBUG: Loaded source code:\n%s\n", buffer);
#endif

    return buffer;
}
