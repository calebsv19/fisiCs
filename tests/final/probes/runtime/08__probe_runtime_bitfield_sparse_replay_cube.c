#include <stdio.h>

struct Bits {
    unsigned a : 2;
    unsigned b : 4;
    unsigned c : 3;
    unsigned d : 5;
    unsigned e : 1;
};

struct Grid {
    struct Bits lanes[2][2][2];
    unsigned tag;
};

static unsigned checksum(const struct Grid *grids, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Bits *bits = &grids[i].lanes[x][y][z];
                    acc = acc * 37u +
                          bits->a * 11u +
                          bits->b * 7u +
                          bits->c * 5u +
                          bits->d * 3u +
                          bits->e +
                          grids[i].tag;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .lanes[0][0][0] = {
                .a = 1,
                .d = 9,
            },
            .lanes[0][0][0].b = 6,
            .lanes[1][1][1].c = 5,
            .lanes[1][0][1].e = 1,
            .tag = 2,
        },
        [1] = {
            .lanes[0][1][0] = {
                .b = 8,
                .d = 11,
            },
            .lanes[0][1][0].a = 2,
            .lanes[1][1][0].c = 4,
            .lanes[1][1][0].d = 13,
            .lanes[0][0][1].e = 1,
            .tag = 5,
        },
    };

    printf("%u %u %u %u\n",
           grids[0].lanes[0][0][0].b,
           grids[0].lanes[1][1][1].c,
           grids[1].lanes[0][1][0].a,
           checksum(grids, 2));
    return 0;
}
