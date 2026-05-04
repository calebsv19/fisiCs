#include <stdio.h>

struct Bits {
    unsigned a : 3;
    unsigned pad0 : 1;
    unsigned b : 4;
    unsigned c : 5;
    unsigned d : 3;
    unsigned e : 1;
};

struct Hyper {
    struct Bits lanes[2][2][2];
    unsigned tail;
};

static unsigned checksum(const struct Hyper *hypers, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Bits *bits = &hypers[i].lanes[x][y][z];
                    acc = acc * 61u +
                          bits->a * 17u +
                          bits->b * 11u +
                          bits->c * 7u +
                          bits->d * 5u +
                          bits->e * 3u +
                          hypers[i].tail;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct Hyper hypers[2] = {
        [0] = {
            .lanes[0][0][0].a = 5,
            .lanes[0][0][0].d = 3,
            .lanes[0] = {
                [1] = {
                    [0] = {
                        .b = 9,
                        .c = 12,
                    },
                    [1] = {
                        .a = 2,
                        .e = 1,
                    },
                },
            },
            .tail = 3,
        },
        [1] = {
            .lanes[1][1][1].b = 7,
            .lanes[1][1][1].c = 15,
            .lanes[1] = {
                [0] = {
                    [0] = {
                        .a = 6,
                        .d = 4,
                    },
                    [1] = {
                        .b = 5,
                        .e = 1,
                    },
                },
            },
            .tail = 6,
        },
    };

    printf("%u %u %u %u\n",
           hypers[0].lanes[0][1][0].c,
           hypers[0].lanes[0][1][1].e,
           hypers[1].lanes[1][0][0].a,
           checksum(hypers, 2));
    return 0;
}
