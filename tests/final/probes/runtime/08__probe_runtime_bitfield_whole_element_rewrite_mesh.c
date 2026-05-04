#include <stdio.h>

struct Bits {
    unsigned a : 2;
    unsigned b : 4;
    unsigned c : 3;
    unsigned d : 5;
};

struct Grid {
    struct Bits lanes[2][2];
    unsigned tag;
};

static unsigned checksum(const struct Grid *grids, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Bits *bits = &grids[i].lanes[r][c];
                acc = acc * 31u +
                      bits->a * 13u +
                      bits->b * 7u +
                      bits->c * 5u +
                      bits->d * 3u +
                      grids[i].tag;
            }
        }
    }
    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .lanes[0][0].d = 9,
            .lanes[0][0] = {
                .a = 1,
                .b = 6,
            },
            .lanes[1][1] = {
                .b = 8,
                .d = 12,
            },
            .tag = 2,
        },
        [1] = {
            .lanes[0][1].c = 4,
            .lanes[0][1] = {
                .a = 2,
                .d = 11,
            },
            .lanes[1][0] = {
                .b = 5,
                .c = 7,
            },
            .tag = 4,
        },
    };

    printf("%u %u %u %u\n",
           grids[0].lanes[0][0].b,
           grids[0].lanes[1][1].d,
           grids[1].lanes[0][1].a,
           checksum(grids, 2));
    return 0;
}
