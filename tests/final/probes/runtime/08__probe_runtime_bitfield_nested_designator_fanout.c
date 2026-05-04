#include <stdio.h>

struct Bits {
    unsigned a : 2;
    unsigned b : 3;
    unsigned c : 3;
};

struct Row {
    unsigned tag;
    struct Bits lanes[2];
    unsigned tail;
};

static unsigned checksum(const struct Row *rows, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < 2; ++j) {
            acc = acc * 31u + rows[i].tag * 11u + rows[i].lanes[j].a * 7u +
                  rows[i].lanes[j].b * 5u + rows[i].lanes[j].c * 3u + rows[i].tail;
        }
    }
    return acc;
}

int main(void) {
    struct Row rows[3] = {
        [1] = {
            .lanes[1].b = 5,
            .lanes[0].c = 6,
            .tail = 4,
        },
        [2] = {
            .tag = 3,
            .lanes[1] = {
                .a = 2,
                .c = 7,
            },
        },
    };

    printf(
        "%u %u %u %u\n",
        rows[0].tag + rows[0].lanes[0].a + rows[0].lanes[0].b + rows[0].lanes[0].c + rows[0].tail,
        rows[1].lanes[1].b,
        rows[2].lanes[1].c,
        checksum(rows, 3)
    );
    return 0;
}
