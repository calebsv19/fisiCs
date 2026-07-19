#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void daw_g_fail(const char* message) {
    fprintf(stderr, "daw_stage_g: %s\n", message ? message : "failure");
    exit(1);
}

static inline void daw_g_expect(int condition, const char* message) {
    if (!condition) daw_g_fail(message);
}

static inline void daw_g_trace(const char* checkpoint, const char* fields) {
    printf("TRACE|1|%s|%s|result=1\n", checkpoint, fields);
}

static inline uint64_t daw_g_hash_bytes(uint64_t hash, const void* bytes, size_t size) {
    const unsigned char* p = (const unsigned char*)bytes;
    for (size_t i = 0; i < size; ++i) {
        hash ^= (uint64_t)p[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static inline uint64_t daw_g_hash_u64(uint64_t hash, uint64_t value) {
    return daw_g_hash_bytes(hash, &value, sizeof(value));
}

static inline uint64_t daw_g_hash_text(uint64_t hash, const char* text) {
    return daw_g_hash_bytes(hash, text, text ? strlen(text) : 0u);
}

static inline void daw_g_write_text(const char* path, const char* text) {
    FILE* file = fopen(path, "wb");
    daw_g_expect(file != NULL, "open canonical artifact");
    daw_g_expect(fputs(text, file) >= 0, "write canonical artifact");
    daw_g_expect(fclose(file) == 0, "close canonical artifact");
}
