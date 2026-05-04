#include <stdio.h>

static int alpha = 4;
static int beta = 6;
static int gamma = 9;

static int inc_two(int x) {
    return x + 2;
}

static int mul_four(int x) {
    return x * 4;
}

struct Node {
    int *slot;
    int (*fn)(int);
};

struct Table {
    struct Node nodes[2][2];
    int *aliases[2];
    int (*callbacks[2])(int);
};

static struct Table tables[2] = {
    [0] = {
        .nodes[0][0].slot = &alpha,
        .nodes[0][0].fn = inc_two,
        .nodes[1][1].slot = &gamma,
        .aliases[0] = &beta,
        .callbacks[1] = mul_four,
    },
    [1] = {
        .nodes[0][1] = {
            .slot = &beta,
            .fn = mul_four,
        },
        .nodes[1][0].slot = &alpha,
        .nodes[1][0].fn = inc_two,
        .aliases[1] = &gamma,
        .callbacks[0] = inc_two,
        .callbacks[1] = mul_four,
    },
};

static unsigned checksum(void) {
    unsigned acc = 0;

    for (int i = 0; i < 2; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Node *node = &tables[i].nodes[r][c];
                int slot = node->slot ? *node->slot : 0;
                int fnv = node->fn ? node->fn(r + c + 1) : 0;
                int alias = tables[i].aliases[r] ? *tables[i].aliases[r] : 0;
                int cb = tables[i].callbacks[c] ? tables[i].callbacks[c](i + 2) : 0;
                acc = acc * 29u + (unsigned)(slot * 7 + fnv * 5 + alias * 3 + cb);
            }
        }
    }
    return acc;
}

int main(void) {
    printf("%d %d %d %u\n",
           tables[0].nodes[0][0].fn ? tables[0].nodes[0][0].fn(3) : 0,
           tables[1].aliases[1] ? *tables[1].aliases[1] : 0,
           tables[1].callbacks[0] ? tables[1].callbacks[0](5) : 0,
           checksum());
    return 0;
}
