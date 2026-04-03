#include "binary__corpus_fragment_symbol_pool.h"

#define POOL_CAP 16u

static SymEntry g_pool[POOL_CAP];
static unsigned g_count = 0u;

static int slice_eq(const char *a, size_t na, const char *b, size_t nb) {
    size_t i = 0;
    if (na != nb) return 0;
    while (i < na) {
        if (a[i] != b[i]) return 0;
        ++i;
    }
    return 1;
}

int sym_pool_find(const char *name, size_t len) {
    unsigned i = 0;
    while (i < g_count) {
        if (slice_eq(g_pool[i].name, g_pool[i].len, name, len)) {
            return (int)g_pool[i].id;
        }
        ++i;
    }
    return -1;
}

int sym_pool_add(const char *name, size_t len) {
    unsigned slot;
    if (g_count >= POOL_CAP) return -1;
    if (sym_pool_find(name, len) >= 0) return 0;

    slot = sym_hash(name, len) % POOL_CAP;
    while (g_pool[slot].name) {
        slot = (slot + 1u) % POOL_CAP;
    }

    g_pool[slot].name = name;
    g_pool[slot].len = len;
    g_pool[slot].id = g_count + 1u;
    ++g_count;
    return (int)g_pool[slot].id;
}

unsigned sym_pool_count(void) {
    return g_count;
}
