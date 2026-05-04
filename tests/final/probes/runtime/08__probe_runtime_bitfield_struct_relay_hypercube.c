#include <stdio.h>

struct Bits {
    unsigned a : 3;
    unsigned pad0 : 1;
    unsigned b : 4;
    unsigned c : 5;
    unsigned d : 3;
    unsigned e : 1;
};

struct Cell {
    struct Bits bits;
    unsigned mark;
};

struct Grid {
    struct Cell cells[2][2][2];
    unsigned tail;
};

static unsigned checksum(const struct Grid grids[2]) {
    unsigned acc = 0;

    for (int g = 0; g < 2; ++g) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Cell *cell = &grids[g].cells[x][y][z];
                    acc = acc * 71u +
                          cell->bits.a * 17u +
                          cell->bits.b * 13u +
                          cell->bits.c * 11u +
                          cell->bits.d * 7u +
                          cell->bits.e * 5u +
                          cell->mark * 3u +
                          grids[g].tail;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .cells[0][0][0].bits.a = 5,
            .cells[0][0][0].bits.d = 3,
            .cells[0][0][0].mark = 1,
            .cells[0] = {
                [1] = {
                    [0] = { .bits = { .b = 9, .c = 12 }, .mark = 4 },
                    [1] = { .bits = { .a = 2, .e = 1 }, .mark = 6 },
                },
            },
            .tail = 3,
        },
        [1] = {
            .cells[1][1][1].bits.b = 7,
            .cells[1][1][1].bits.c = 15,
            .cells[1][1][1].mark = 2,
            .cells[1] = {
                [0] = {
                    [0] = { .bits = { .a = 6, .d = 4 }, .mark = 8 },
                    [1] = { .bits = { .b = 5, .e = 1 }, .mark = 9 },
                },
            },
            .tail = 6,
        },
    };

    printf("%u %u %u %u\n",
           grids[0].cells[0][1][0].bits.c,
           grids[0].cells[0][1][1].bits.e,
           grids[1].cells[1][0][0].bits.a,
           checksum(grids));
    return 0;
}
