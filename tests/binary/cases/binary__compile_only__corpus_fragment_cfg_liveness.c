#include <stddef.h>

typedef struct {
    unsigned use_mask;
    unsigned def_mask;
    unsigned in_mask;
    unsigned out_mask;
} Block;

typedef struct {
    size_t from;
    size_t to;
} Edge;

static unsigned transfer(unsigned out_mask, const Block *b) {
    return b->use_mask | (out_mask & ~b->def_mask);
}

int liveness_step(Block *blocks, size_t nblocks, const Edge *edges, size_t nedges) {
    size_t i;
    int changed = 0;
    for (i = 0; i < nblocks; ++i) {
        unsigned out_new = 0;
        size_t e;
        for (e = 0; e < nedges; ++e) {
            if (edges[e].from == i) out_new |= blocks[edges[e].to].in_mask;
        }
        if (out_new != blocks[i].out_mask) {
            blocks[i].out_mask = out_new;
            changed = 1;
        }
        {
            unsigned in_new = transfer(out_new, &blocks[i]);
            if (in_new != blocks[i].in_mask) {
                blocks[i].in_mask = in_new;
                changed = 1;
            }
        }
    }
    return changed;
}
