#include "binary__corpus_fragment_symbol_pool.h"

unsigned sym_hash(const char *s, size_t n) {
    size_t i = 0;
    unsigned h = 2166136261u;
    while (i < n) {
        h ^= (unsigned)(unsigned char)s[i];
        h *= 16777619u;
        ++i;
    }
    return h;
}
