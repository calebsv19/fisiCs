#include <stddef.h>
#include <stdio.h>

struct Leaf {
    unsigned char tag;
    unsigned char bytes[3];
};

union Arm {
    struct Leaf leaf;
    unsigned char raw[4];
};

struct Holder {
    union Arm arms[2];
    unsigned char tail;
};

static unsigned checksum(const struct Holder *holder) {
    unsigned acc = holder->tail;

    for (int i = 0; i < 2; ++i) {
        acc = acc * 31u + holder->arms[i].raw[0];
        acc = acc * 23u + holder->arms[i].raw[1];
        acc = acc * 17u + holder->arms[i].raw[3];
    }

    return acc;
}

int main(void) {
    struct Holder holder = {
        .arms[0].leaf = { .tag = 3, .bytes = { 5 } },
        .arms[1].raw = { 7, 11 },
        .tail = 13,
    };

    holder.arms[1] = (union Arm){ .leaf = { .tag = 17, .bytes = { [2] = 19 } } };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Holder, arms),
           (unsigned)offsetof(struct Holder, tail),
           (unsigned)holder.arms[0].raw[2],
           (unsigned)holder.arms[1].raw[0],
           (unsigned)holder.arms[1].raw[1],
           checksum(&holder));
    return 0;
}
