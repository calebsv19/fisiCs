#include <stdio.h>

static int alpha = 3;
static int beta = 5;
static int gamma = 8;
static int delta = 13;

static int add_two(int x) { return x + 2; }
static int add_four(int x) { return x + 4; }
static int mul_three(int x) { return x * 3; }

struct Leaf {
    int value;
    int *slot;
    int (*fn)(int);
};

struct Block {
    struct Leaf cells[2][2][2];
    int *aliases[2][2];
};

static struct Block blocks[2] = {
    [0] = {
        .cells[0][0][0].value = 1,
        .cells[0][0][0] = {
            .slot = &beta,
            .fn = mul_three,
        },
        .cells[0][0][1] = {
            .value = 4,
            .slot = &gamma,
            .fn = add_two,
        },
        .cells[1] = {
            [0] = {
                [1] = {
                    .value = 6,
                    .slot = &delta,
                    .fn = add_four,
                },
            },
            [1] = {
                [0] = {
                    .slot = &alpha,
                    .fn = add_two,
                },
            },
        },
        .aliases[1][0] = &delta,
        .aliases[1][1] = &beta,
    },
    [1] = {
        .cells[1][1][1].value = 9,
        .cells[1][1][1] = {
            .slot = &alpha,
            .fn = add_four,
        },
        .cells[0] = {
            [0] = {
                [0] = {
                    .value = 2,
                    .slot = &gamma,
                    .fn = add_two,
                },
            },
            [1] = {
                [1] = {
                    .slot = &beta,
                    .fn = mul_three,
                },
            },
        },
        .aliases[0][0] = &beta,
        .aliases[0][1] = &gamma,
        .aliases[1][1] = &alpha,
    },
};

static unsigned checksum(void) {
    unsigned acc = 0;

    for (int b = 0; b < 2; ++b) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Leaf *leaf = &blocks[b].cells[x][y][z];
                    int slot = leaf->slot ? *leaf->slot : 0;
                    int fnv = leaf->fn ? leaf->fn(x + y + z + 1) : leaf->value;
                    int alias = blocks[b].aliases[y][z] ? *blocks[b].aliases[y][z] : 0;
                    acc = acc * 41u + (unsigned)(leaf->value * 11 + slot * 7 + fnv * 5 + alias);
                }
            }
        }
    }
    return acc;
}

int main(void) {
    printf("%d %d %d %u\n",
           blocks[0].cells[0][0][0].fn ? blocks[0].cells[0][0][0].fn(2) : 0,
           blocks[0].aliases[1][0] ? *blocks[0].aliases[1][0] : 0,
           blocks[1].cells[1][1][1].fn ? blocks[1].cells[1][1][1].fn(3) : 0,
           checksum());
    return 0;
}
