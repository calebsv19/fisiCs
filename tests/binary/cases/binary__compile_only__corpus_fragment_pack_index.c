#include <stddef.h>

typedef struct {
    unsigned first;
    unsigned count;
} Span;

typedef struct {
    Span spans[8];
    unsigned used;
} SpanPack;

unsigned spanpack_push(SpanPack *pack, unsigned first, unsigned count) {
    if (pack->used >= 8u) return 0u;
    pack->spans[pack->used].first = first;
    pack->spans[pack->used].count = count;
    ++pack->used;
    return 1u;
}

size_t spanpack_total(const SpanPack *pack) {
    size_t i = 0;
    size_t total = 0;
    while (i < pack->used) {
        total += (size_t)pack->spans[i].count;
        ++i;
    }
    return total;
}
