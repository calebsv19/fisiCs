#pragma once

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static uint64_t ps_stageg_hash_bytes(uint64_t hash, const void *data, size_t size) {
    const unsigned char *bytes = (const unsigned char *)data;
    size_t i;
    for (i = 0u; i < size; ++i) {
        hash ^= (uint64_t)bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t ps_stageg_hash_text(uint64_t hash, const char *text) {
    return ps_stageg_hash_bytes(hash, text ? text : "", text ? strlen(text) : 0u);
}

static void ps_stageg_trace(const char *checkpoint, const char *detail, uint64_t digest) {
    printf("TRACE|1|%s|detail=%s|digest=%016llx|result=1\n",
           checkpoint, detail ? detail : "ok", (unsigned long long)digest);
}

static int ps_stageg_mkdir(const char *path) {
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

static int ps_stageg_write_text(const char *path, const char *text) {
    FILE *file = fopen(path, "wb");
    size_t size = text ? strlen(text) : 0u;
    int ok;
    if (!file) return 0;
    ok = size == 0u || fwrite(text, 1u, size, file) == size;
    return fclose(file) == 0 && ok;
}

static char *ps_stageg_read_text(const char *path) {
    FILE *file = fopen(path, "rb");
    long size;
    char *text;
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)size + 1u);
    if (!text) {
        fclose(file);
        return NULL;
    }
    if (size > 0 && fread(text, 1u, (size_t)size, file) != (size_t)size) {
        free(text);
        fclose(file);
        return NULL;
    }
    text[size] = '\0';
    fclose(file);
    return text;
}
