#include <stdio.h>

typedef struct Axis5W9Op {
    unsigned int key;
    int delta;
} Axis5W9Op;

typedef struct Axis5W9Cell {
    unsigned int key;
    int value;
    unsigned int live;
} Axis5W9Cell;

static unsigned int axis5_w9_mix(unsigned int h, unsigned int v) {
    h ^= v + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
}

static void axis5_w9_sparse_fold(const Axis5W9Op* ops, int n, Axis5W9Cell* out) {
    for (int i = 0; i < 8; ++i) {
        out[i].key = (unsigned int)i;
        out[i].value = 0;
        out[i].live = 0u;
    }
    for (int i = 0; i < n; ++i) {
        unsigned int idx = ops[i].key & 7u;
        out[idx].value += ops[i].delta;
        out[idx].live = (out[idx].value != 0) ? 1u : 0u;
    }
}

static void axis5_w9_dense_fold(const Axis5W9Op* ops, int n, int dense[8]) {
    for (int i = 0; i < 8; ++i) dense[i] = 0;
    for (int i = 0; i < n; ++i) dense[ops[i].key & 7u] += ops[i].delta;
}

static unsigned int axis5_w9_sparse_sig(const Axis5W9Cell* sparse) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < 8; ++i) {
        if (sparse[i].live == 0u) continue;
        h = axis5_w9_mix(h, sparse[i].key & 31u);
        h = axis5_w9_mix(h, (unsigned int)(sparse[i].value & 0xffff));
    }
    return h;
}

static unsigned int axis5_w9_dense_sig(const int dense[8]) {
    unsigned int h = 2166136261u;
    for (unsigned int i = 0; i < 8u; ++i) {
        if (dense[i] == 0) continue;
        h = axis5_w9_mix(h, i & 31u);
        h = axis5_w9_mix(h, (unsigned int)(dense[i] & 0xffff));
    }
    return h;
}

int main(void) {
    const Axis5W9Op ops[] = {
        {1u, +7}, {4u, +3}, {1u, -2}, {6u, +9}, {4u, -3}, {0u, +5},
        {6u, -4}, {2u, +8}, {0u, -5}, {2u, -1}, {3u, +6}, {3u, -6},
    };
    Axis5W9Cell sparse[8];
    int dense[8];

    axis5_w9_sparse_fold(ops, 12, sparse);
    axis5_w9_dense_fold(ops, 12, dense);

    const unsigned int sig_sparse = axis5_w9_sparse_sig(sparse);
    const unsigned int sig_dense = axis5_w9_dense_sig(dense);
    const unsigned int same = (sig_sparse == sig_dense) ? 1u : 0u;

    printf("%u %u %u\n", sig_sparse, sig_dense, same);
    return 0;
}
