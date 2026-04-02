#ifndef PROBE_RUNTIME_CLANG_GCC_TRI_DIFF_HEADER_LAYOUT_FOLD_MATRIX_H
#define PROBE_RUNTIME_CLANG_GCC_TRI_DIFF_HEADER_LAYOUT_FOLD_MATRIX_H

typedef struct {
    unsigned char tag;
    unsigned short span;
    unsigned value;
    int bias;
} LayoutCell;

static unsigned fold_layout_cell(const LayoutCell *cell, unsigned salt) {
    unsigned acc = (unsigned)cell->tag * 131u + (unsigned)cell->span * 17u;
    acc ^= cell->value + salt * 19u;
    acc ^= (unsigned)(cell->bias + 0x4000);
    acc = (acc << 5u) | (acc >> 27u);
    return acc ^ (acc >> 11u);
}

static unsigned mix_layout_rows(const LayoutCell *cells, unsigned n, unsigned salt0, unsigned salt1) {
    unsigned a = salt0;
    unsigned b = salt1;
    unsigned i;
    for (i = 0u; i < n; ++i) {
        unsigned v = fold_layout_cell(&cells[i], i + 1u);
        a ^= v + i * 23u;
        b += (v ^ (a >> 3u)) + i * 31u;
    }
    return a ^ (b * 3u + 97u);
}

#endif
