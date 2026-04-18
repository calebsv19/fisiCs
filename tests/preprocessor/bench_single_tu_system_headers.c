#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct BenchRecord {
    size_t length;
    uint32_t hash;
    bool ready;
} BenchRecord;

static size_t bench_copy(char* dst, size_t capacity, const char* src) {
    size_t length = strlen(src);
    if (length + 1 > capacity) {
        return 0;
    }
    memcpy(dst, src, length + 1);
    return length;
}

static uint32_t bench_fold(const char* text) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        hash ^= (uint32_t)(unsigned char)text[i];
        hash *= 16777619u;
    }
    return hash;
}

int main(void) {
    char buffer[64];
    BenchRecord record = {0};

    record.length = bench_copy(buffer, sizeof(buffer), "fisics preprocessor bench");
    record.hash = bench_fold(buffer);
    record.ready = (record.length != 0u);

    if (!record.ready) {
        return EXIT_FAILURE;
    }

    puts(buffer);
    return record.hash == 0u ? EXIT_FAILURE : EXIT_SUCCESS;
}
