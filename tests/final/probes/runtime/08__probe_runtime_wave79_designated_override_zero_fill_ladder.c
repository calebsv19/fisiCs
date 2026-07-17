#include <stddef.h>
#include <stdio.h>

struct Leaf {
    unsigned char code;
    unsigned char values[3];
};

struct Node {
    struct Leaf leaves[2];
    unsigned char seal;
};

struct Forest {
    struct Node nodes[2];
    unsigned char final;
};

static unsigned checksum(const struct Forest *forest) {
    unsigned acc = forest->final;

    for (int n = 0; n < 2; ++n) {
        acc = acc * 43u + forest->nodes[n].seal;
        for (int l = 0; l < 2; ++l) {
            const struct Leaf *leaf = &forest->nodes[n].leaves[l];
            acc = acc * 37u + leaf->code;
            acc = acc * 31u + leaf->values[0];
            acc = acc * 29u + leaf->values[2];
        }
    }

    return acc;
}

int main(void) {
    struct Forest forest = {
        .nodes[0].leaves[0] = { .code = 5, .values = { 7, 11, 13 } },
        .nodes[0] = {
            .leaves = {
                [1] = { .code = 17, .values = { [2] = 19 } },
            },
            .seal = 23,
        },
        .nodes[1].leaves[0].values[1] = 29,
        .nodes[1].leaves[0].code = 31,
        .final = 37,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Forest, final),
           (unsigned)forest.nodes[0].leaves[0].code,
           (unsigned)forest.nodes[0].leaves[1].values[0],
           (unsigned)forest.nodes[0].leaves[1].values[2],
           (unsigned)forest.nodes[1].leaves[0].values[0],
           checksum(&forest));
    return 0;
}
