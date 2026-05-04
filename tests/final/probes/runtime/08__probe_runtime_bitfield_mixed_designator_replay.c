#include <stdio.h>

struct Bits {
    unsigned a : 2;
    unsigned b : 3;
    unsigned c : 3;
};

struct Entry {
    unsigned tag;
    struct Bits lanes[2];
    unsigned tail;
};

static unsigned checksum(const struct Entry *entries, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < 2; ++j) {
            acc = acc * 41u +
                  entries[i].tag * 13u +
                  entries[i].lanes[j].a * 7u +
                  entries[i].lanes[j].b * 5u +
                  entries[i].lanes[j].c * 3u +
                  entries[i].tail;
        }
    }
    return acc;
}

int main(void) {
    struct Entry entries[3] = {
        [0] = {
            .tag = 1,
            .lanes[0] = {
                .a = 1,
                .c = 4,
            },
            .lanes[0].b = 3,
            .lanes[1].c = 7,
        },
        [2] = {
            .lanes[1] = {
                .b = 5,
            },
            .lanes[1].a = 2,
            .tail = 9,
        },
    };

    printf("%u %u %u %u\n",
           entries[0].lanes[0].b,
           entries[0].lanes[1].c,
           entries[2].lanes[1].a,
           checksum(entries, 3));
    return 0;
}
