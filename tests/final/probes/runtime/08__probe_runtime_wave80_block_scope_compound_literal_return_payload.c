#include <stddef.h>
#include <stdio.h>

union Atom {
    unsigned char bytes[4];
    int scalar;
};

struct Pair {
    unsigned char id;
    union Atom atom;
};

struct Box {
    struct Pair pairs[3];
    unsigned char tail;
};

static struct Pair make_pair(int selector) {
    if (selector == 0) {
        return (struct Pair){ .id = 5, .atom.bytes = { [1] = 7, [3] = 11 } };
    }

    return (struct Pair){ .id = 13, .atom = (union Atom){ .bytes = { 17, 19, 23, 0 } } };
}

static unsigned checksum(struct Box box) {
    unsigned acc = box.tail;

    for (int i = 0; i < 3; ++i) {
        const struct Pair *pair = &box.pairs[i];
        acc = acc * 47u + pair->id;
        acc = acc * 43u + pair->atom.bytes[0];
        acc = acc * 41u + pair->atom.bytes[1];
        acc = acc * 37u + pair->atom.bytes[3];
    }

    return acc;
}

int main(void) {
    struct Box box = {
        .pairs[0] = make_pair(0),
        .pairs[1].atom.bytes[2] = 29,
        .tail = 31,
    };

    box.pairs[1] = (struct Pair){ .id = 37, .atom.bytes = { [0] = 41, [2] = 43 } };
    box.pairs[2] = make_pair(1);
    box.pairs[2].atom = (union Atom){ .bytes = { [1] = 53, [3] = 59 } };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Box, tail),
           (unsigned)box.pairs[0].atom.bytes[0],
           (unsigned)box.pairs[1].atom.bytes[2],
           (unsigned)box.pairs[2].atom.bytes[0],
           (unsigned)box.pairs[2].atom.bytes[3],
           checksum(box));
    return 0;
}
