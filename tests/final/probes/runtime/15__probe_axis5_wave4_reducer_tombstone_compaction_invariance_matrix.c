#include <stdio.h>

typedef struct Axis5W4Cell {
    unsigned int key;
    unsigned int value;
    unsigned int live;
} Axis5W4Cell;

static unsigned int axis5_w4_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w4_cell_reset(Axis5W4Cell* c) {
    c->value = 0u;
    c->live = 0u;
}

static void axis5_w4_apply_set(Axis5W4Cell* table, unsigned int key, unsigned int value) {
    unsigned int idx = key & 7u;
    table[idx].key = idx;
    table[idx].value = value & 0xffffu;
    table[idx].live = 1u;
}

static void axis5_w4_apply_delete(Axis5W4Cell* table, unsigned int key) {
    unsigned int idx = key & 7u;
    table[idx].key = idx;
    table[idx].live = 0u;
}

static unsigned int axis5_w4_sparse_sig(const Axis5W4Cell* table) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < 8; ++i) {
        if (table[i].live == 0u) continue;
        h = axis5_w4_mix(h, table[i].key & 31u);
        h = axis5_w4_mix(h, table[i].value & 0xffffu);
    }
    return h;
}

static int axis5_w4_compact_live(const Axis5W4Cell* table, Axis5W4Cell* out) {
    int n = 0;
    for (int i = 0; i < 8; ++i) {
        if (table[i].live == 0u) continue;
        out[n++] = table[i];
    }
    return n;
}

static unsigned int axis5_w4_compact_sig(const Axis5W4Cell* compacted, int n) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; ++i) {
        h = axis5_w4_mix(h, compacted[i].key & 31u);
        h = axis5_w4_mix(h, compacted[i].value & 0xffffu);
    }
    return h;
}

int main(void) {
    Axis5W4Cell table[8];
    Axis5W4Cell compacted[8];
    for (int i = 0; i < 8; ++i) {
        table[i].key = (unsigned int)i;
        axis5_w4_cell_reset(&table[i]);
    }

    axis5_w4_apply_set(table, 1u, 7u);
    axis5_w4_apply_set(table, 3u, 9u);
    axis5_w4_apply_delete(table, 1u);
    axis5_w4_apply_set(table, 5u, 11u);
    axis5_w4_apply_set(table, 1u, 12u);
    axis5_w4_apply_delete(table, 3u);
    axis5_w4_apply_set(table, 2u, 6u);
    axis5_w4_apply_set(table, 7u, 4u);
    axis5_w4_apply_delete(table, 2u);
    axis5_w4_apply_set(table, 3u, 15u);

    const unsigned int sig_sparse = axis5_w4_sparse_sig(table);
    const int n_compact = axis5_w4_compact_live(table, compacted);
    const unsigned int sig_compact = axis5_w4_compact_sig(compacted, n_compact);
    const unsigned int same = (sig_sparse == sig_compact) ? 1u : 0u;

    printf("%u %u %d %u\n", sig_sparse, sig_compact, n_compact, same);
    return 0;
}
