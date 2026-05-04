#include <stdio.h>

struct Bits {
    unsigned a : 2;
    unsigned pad0 : 2;
    unsigned b : 4;
    unsigned c : 3;
    unsigned d : 5;
    unsigned e : 1;
};

struct Cube {
    struct Bits lanes[2][2][2];
    unsigned tail;
};

static unsigned checksum(const struct Cube *cubes, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Bits *bits = &cubes[i].lanes[x][y][z];
                    acc = acc * 53u +
                          bits->a * 17u +
                          bits->b * 11u +
                          bits->c * 7u +
                          bits->d * 5u +
                          bits->e * 3u +
                          cubes[i].tail;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct Cube cubes[2] = {
        [0] = {
            .lanes[0][0][0] = {
                .a = 1,
                .c = 3,
            },
            .lanes[0][0][0].b = 9,
            .lanes[0][1][1].d = 12,
            .lanes[1][0][1].e = 1,
            .tail = 2,
        },
        [1] = {
            .lanes[0][1][0] = {
                .b = 6,
                .d = 8,
            },
            .lanes[0][1][0].a = 2,
            .lanes[1][1][1].c = 5,
            .lanes[1][1][1].d = 17,
            .lanes[1][0][0].e = 1,
            .tail = 4,
        },
    };

    printf("%u %u %u %u\n",
           cubes[0].lanes[0][0][0].b,
           cubes[0].lanes[0][1][1].d,
           cubes[1].lanes[0][1][0].a,
           checksum(cubes, 2));
    return 0;
}
