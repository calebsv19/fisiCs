#ifndef IDE_STAGE_G_COMMON_H
#define IDE_STAGE_G_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ide_g_fail(const char* message) {
    fprintf(stderr, "ide-stage-g: %s\n", message ? message : "failure");
    exit(1);
}

static void ide_g_expect(int condition, const char* message) {
    if (!condition) {
        ide_g_fail(message);
    }
}

static void ide_g_trace(const char* checkpoint, const char* fields) {
    printf("TRACE|1|%s|%s|result=1\n", checkpoint, fields);
}

static void ide_g_write_text(const char* path, const char* text) {
    FILE* out = fopen(path, "wb");
    ide_g_expect(out != NULL, "open canonical artifact");
    size_t size = strlen(text);
    ide_g_expect(fwrite(text, 1, size, out) == size, "write canonical artifact");
    ide_g_expect(fclose(out) == 0, "close canonical artifact");
}

static uint64_t ide_g_hash_u64(uint64_t hash, uint64_t value) {
    for (unsigned int i = 0; i < 8; ++i) {
        hash ^= (unsigned char)((value >> (i * 8u)) & 0xffu);
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t ide_g_hash_text(uint64_t hash, const char* text) {
    const unsigned char* cursor = (const unsigned char*)(text ? text : "");
    while (*cursor) {
        hash ^= *cursor++;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

#endif
