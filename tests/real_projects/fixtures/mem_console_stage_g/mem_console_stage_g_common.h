#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t mc_stageg_hash_bytes(uint64_t hash, const void *data, size_t size) {
    const unsigned char *bytes = (const unsigned char *)data;
    size_t i;
    for (i = 0u; i < size; ++i) {
        hash ^= (uint64_t)bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t mc_stageg_hash_text(uint64_t hash, const char *text) {
    return mc_stageg_hash_bytes(hash, text ? text : "", text ? strlen(text) : 0u);
}

static void mc_stageg_trace(const char *checkpoint, const char *detail, uint64_t digest) {
    printf("TRACE|1|%s|detail=%s|digest=%016llx|result=1\n",
           checkpoint, detail ? detail : "ok", (unsigned long long)digest);
}

static int mc_stageg_write_text(const char *path, const char *text) {
    FILE *file = fopen(path, "wb");
    size_t size = text ? strlen(text) : 0u;
    int ok;
    if (!file) return 0;
    ok = size == 0u || fwrite(text, 1u, size, file) == size;
    return fclose(file) == 0 && ok;
}
