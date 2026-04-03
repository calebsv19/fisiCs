#include <stddef.h>

typedef struct {
    char *base;
    size_t capacity;
    size_t used;
} Arena;

char *arena_alloc(Arena *a, size_t n) {
    char *out;
    if (!a) return (char *)0;
    if (n > a->capacity - a->used) return (char *)0;
    out = a->base + a->used;
    a->used += n;
    return out;
}

int arena_push_cstr(Arena *a, const char *s) {
    size_t len = 0;
    char *dst;
    if (!s) return -1;
    while (s[len]) ++len;
    dst = arena_alloc(a, len + 1u);
    if (!dst) return -1;
    for (len = 0; s[len]; ++len) {
        dst[len] = s[len];
    }
    dst[len] = '\0';
    return 0;
}
