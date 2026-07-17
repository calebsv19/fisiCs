#include <stddef.h>
#include <stdio.h>

struct Leaf {
    unsigned char code;
    unsigned char bytes[4];
};

union Node {
    struct Leaf leaf;
    unsigned char raw[5];
};

struct Branch {
    union Node nodes[2];
    unsigned char mark;
};

struct Tree {
    struct Branch branches[3];
};

static unsigned checksum(const struct Tree *tree) {
    unsigned acc = 0;

    for (int b = 0; b < 3; ++b) {
        acc = acc * 31u + tree->branches[b].mark;
        for (int n = 0; n < 2; ++n) {
            acc = acc * 19u + tree->branches[b].nodes[n].raw[0];
            acc = acc * 13u + tree->branches[b].nodes[n].raw[4];
        }
    }

    return acc;
}

int main(void) {
    struct Tree tree = {
        .branches[0].nodes[1].leaf = { .code = 3, .bytes = { [3] = 5 } },
        .branches[1] = {
            .nodes = {
                [0].raw = { 7 },
                [1].leaf = { .bytes = { 11, 13 } },
            },
            .mark = 17,
        },
        .branches[2].mark = 19,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Tree, branches),
           (unsigned)offsetof(struct Branch, mark),
           (unsigned)tree.branches[0].nodes[0].raw[0],
           (unsigned)tree.branches[0].nodes[1].raw[4],
           (unsigned)tree.branches[1].nodes[1].raw[0],
           checksum(&tree));
    return 0;
}
