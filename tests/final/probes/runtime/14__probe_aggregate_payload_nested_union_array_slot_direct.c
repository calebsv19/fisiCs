#include <stdio.h>

typedef struct Cell {
    unsigned char guard;
    union {
        struct {
            unsigned char x;
            unsigned char y;
            unsigned char z;
        };
        unsigned char raw[3];
    };
} Cell;

typedef struct Grid {
    Cell cells[2][2];
    unsigned char trailer;
} Grid;

static unsigned byte_fold_grid(Grid value) {
    unsigned fold = 0u;
    unsigned i;
    unsigned row;
    unsigned col;

    for (row = 0u; row < 2u; ++row) {
        for (col = 0u; col < 2u; ++col) {
            fold += ((unsigned)value.cells[row][col].guard + 1u) * (row + 3u) * (col + 5u);
            for (i = 0u; i < sizeof(value.cells[row][col].raw); ++i) {
                fold += ((unsigned)value.cells[row][col].raw[i] + 1u) * (i + 7u);
                fold ^= (fold << 5) | (fold >> 27);
            }
        }
    }
    fold ^= (unsigned)value.trailer * 257u;
    return fold;
}

static Cell make_cell(unsigned char base) {
    Cell out;
    out.guard = (unsigned char)(0x80u | base);
    out.x = (unsigned char)(base + 0x11u);
    out.y = (unsigned char)(base + 0x22u);
    out.z = (unsigned char)(base + 0x33u);
    return out;
}

int main(void) {
    Grid grid;
    Cell tmp;
    unsigned typed;

    grid.cells[0][0] = make_cell(1u);
    grid.cells[0][1] = make_cell(5u);
    grid.cells[1][0] = make_cell(9u);
    grid.cells[1][1] = make_cell(13u);
    grid.trailer = 0xE7u;

    tmp = grid.cells[0][1];
    tmp.raw[0] ^= 0x3Cu;
    tmp.raw[2] ^= 0xC3u;
    grid.cells[1][0] = tmp;
    grid.cells[0][0].raw[1] = grid.cells[1][1].z;
    grid.cells[1][1].x = grid.cells[1][0].raw[2];

    typed = (unsigned)grid.cells[0][0].guard
        + ((unsigned)grid.cells[0][0].y << 1)
        + ((unsigned)grid.cells[1][0].x << 3)
        + ((unsigned)grid.cells[1][0].z << 5)
        + ((unsigned)grid.cells[1][1].x << 7)
        + ((unsigned)grid.trailer << 9);

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)grid.cells[0][0].y,
           (unsigned)grid.cells[1][0].guard,
           (unsigned)grid.cells[1][0].x,
           (unsigned)grid.cells[1][0].y,
           (unsigned)grid.cells[1][0].z,
           typed,
           byte_fold_grid(grid));
    return 0;
}
