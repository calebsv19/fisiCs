#include <stdio.h>

struct Bits {
    unsigned a : 1;
    unsigned pad : 2;
    unsigned b : 4;
    unsigned c : 3;
    unsigned d : 5;
};

struct Cell {
    struct Bits lanes[2];
    unsigned tag;
};

struct Grid {
    struct Cell cells[2][2];
};

static unsigned checksum(const struct Grid *grids, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Cell *cell = &grids[i].cells[r][c];
                for (int lane = 0; lane < 2; ++lane) {
                    const struct Bits *bits = &cell->lanes[lane];
                    acc = acc * 37u +
                          bits->a * 11u +
                          bits->b * 7u +
                          bits->c * 5u +
                          bits->d * 3u +
                          cell->tag;
                }
            }
        }
    }

    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .cells[0][0].lanes[1].d = 9,
            .cells[0][0].lanes[1] = {
                .a = 1,
                .b = 6,
            },
            .cells[1][1].lanes[0] = {
                .c = 5,
                .d = 11,
            },
            .cells[1][1].lanes[0].b = 3,
            .cells[1][1].tag = 2,
        },
        [1] = {
            .cells[0][1].lanes[0] = {
                .b = 8,
                .d = 7,
            },
            .cells[0][1].lanes[0].a = 1,
            .cells[0][1].lanes[1] = {
                .c = 4,
            },
            .cells[0][1].lanes[1].d = 10,
            .cells[0][1].tag = 5,
        },
    };

    printf("%u %u %u %u\n",
           grids[0].cells[0][0].lanes[1].b,
           grids[0].cells[1][1].lanes[0].d,
           grids[1].cells[0][1].lanes[0].a,
           checksum(grids, 2));
    return 0;
}
