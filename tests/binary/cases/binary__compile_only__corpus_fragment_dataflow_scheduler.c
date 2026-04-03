#include <stddef.h>

typedef struct {
    unsigned id;
    unsigned succ[4];
    unsigned succ_count;
} Block;

typedef struct {
    Block *blocks;
    unsigned count;
} Cfg;

static unsigned contains(const unsigned *items, unsigned n, unsigned v) {
    unsigned i = 0;
    while (i < n) {
        if (items[i] == v) return 1u;
        ++i;
    }
    return 0u;
}

unsigned cfg_linearize(const Cfg *cfg, unsigned *order, unsigned cap) {
    unsigned out = 0;
    unsigned i = 0;
    while (i < cfg->count && out < cap) {
        const Block *b = &cfg->blocks[i];
        if (!contains(order, out, b->id)) {
            order[out++] = b->id;
        }
        ++i;
    }
    return out;
}
