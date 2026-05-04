#include <stdio.h>

struct Bits {
    unsigned lo : 2;
    unsigned pad0 : 2;
    unsigned hi : 4;
    unsigned tag : 3;
    unsigned carry : 5;
};

struct Cube {
    struct Bits rows[2][2][2];
    unsigned seed;
};

static unsigned checksum(const struct Cube *cubes, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Bits *bits = &cubes[i].rows[x][y][z];
                    acc = acc * 67u +
                          bits->lo * 13u +
                          bits->hi * 11u +
                          bits->tag * 7u +
                          bits->carry * 5u +
                          cubes[i].seed;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct Cube cubes[2] = {
        [0] = {
            .rows[0][1][0].lo = 2,
            .rows[0][1][0].carry = 9,
            .rows[0][1] = {
                [0] = {
                    .hi = 6,
                    .tag = 3,
                },
                [1] = {
                    .lo = 1,
                    .carry = 11,
                },
            },
            .seed = 4,
        },
        [1] = {
            .rows[1][0][1].tag = 5,
            .rows[1][0][1].carry = 7,
            .rows[1][0] = {
                [0] = {
                    .lo = 3,
                    .hi = 8,
                },
                [1] = {
                    .hi = 9,
                    .tag = 2,
                },
            },
            .seed = 6,
        },
    };

    printf("%u %u %u %u\n",
           cubes[0].rows[0][1][0].hi,
           cubes[0].rows[0][1][1].carry,
           cubes[1].rows[1][0][0].lo,
           checksum(cubes, 2));
    return 0;
}
