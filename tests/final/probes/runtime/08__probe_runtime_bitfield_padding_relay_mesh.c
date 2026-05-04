#include <stdio.h>

struct Bits {
    unsigned a : 1;
    unsigned pad : 3;
    unsigned b : 4;
    unsigned c : 3;
    unsigned d : 5;
};

struct Block {
    unsigned tag;
    struct Bits lanes[2][2];
    unsigned tail;
};

static unsigned checksum(const struct Block *blocks, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Bits *bits = &blocks[i].lanes[r][c];
                acc = acc * 43u +
                      blocks[i].tag * 13u +
                      bits->a * 11u +
                      bits->b * 7u +
                      bits->c * 5u +
                      bits->d * 3u +
                      blocks[i].tail;
            }
        }
    }
    return acc;
}

int main(void) {
    struct Block blocks[2] = {
        [0] = {
            .tag = 1,
            .lanes[0][0] = {
                .a = 1,
                .c = 3,
            },
            .lanes[0][0].b = 6,
            .lanes[1][1].d = 12,
        },
        [1] = {
            .lanes[0][1] = {
                .b = 9,
                .d = 7,
            },
            .lanes[0][1].a = 1,
            .lanes[1][0].c = 5,
            .tail = 4,
        },
    };

    printf("%u %u %u %u\n",
           blocks[0].lanes[0][0].b,
           blocks[0].lanes[1][1].d,
           blocks[1].lanes[0][1].a,
           checksum(blocks, 2));
    return 0;
}
