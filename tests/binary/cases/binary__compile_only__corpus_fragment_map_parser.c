#include <stddef.h>

typedef struct {
    const char *key;
    int value;
    unsigned flags;
} Entry;

static unsigned hash_key(const char *s) {
    unsigned h = 2166136261u;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 16777619u;
    }
    return h;
}

static int parse_entry(const char *line, Entry *out) {
    unsigned h = hash_key(line);
    out->key = line;
    out->value = (int)(h % 1000u);
    out->flags = (h >> 8u) & 0xffu;
    return (int)(out->flags & 1u);
}

int build_table(const char **lines, size_t n, Entry *out) {
    size_t i;
    int acc = 0;
    for (i = 0; i < n; ++i) {
        acc += parse_entry(lines[i], &out[i]);
    }
    return acc;
}
