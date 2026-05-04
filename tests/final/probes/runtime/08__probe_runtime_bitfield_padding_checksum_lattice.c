#include <stdio.h>

struct Flags {
    unsigned a : 1;
    unsigned b : 2;
    unsigned c : 5;
};

struct Node {
    unsigned tag;
    struct Flags flags;
    unsigned tail;
};

static unsigned checksum(const struct Node *nodes, int count) {
    unsigned acc = 0;

    for (int i = 0; i < count; ++i) {
        acc = acc * 37u + nodes[i].tag * 11u + nodes[i].flags.a * 7u + nodes[i].flags.b * 5u + nodes[i].flags.c * 3u + nodes[i].tail;
    }
    return acc;
}

int main(void) {
    struct Node nodes[3] = {
        [0] = {
            .tail = 4,
            .flags = {
                .c = 9,
            },
            .tag = 1,
        },
        [2] = {
            .flags = {
                .a = 1,
                .b = 3,
            },
            .tail = 7,
        },
    };

    printf(
        "%u %u %u %u\n",
        nodes[1].tag + nodes[1].flags.a + nodes[1].flags.b + nodes[1].flags.c + nodes[1].tail,
        nodes[2].flags.a,
        nodes[2].flags.b,
        checksum(nodes, 3)
    );
    return 0;
}
