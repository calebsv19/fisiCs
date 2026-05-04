#include <stdio.h>

static int alpha = 2;
static int beta = 5;
static int gamma = 8;

static int add_three(int x) {
    return x + 3;
}

static int mul_three(int x) {
    return x * 3;
}

struct Node {
    int *slot;
    int (*fn)(int);
};

struct Table {
    struct Node row[2];
    int *alias;
};

static struct Table tables[2] = {
    [0] = {
        .row[0].slot = &alpha,
        .row[0].fn = add_three,
        .row[0] = {
            .slot = &beta,
            .fn = mul_three,
        },
        .alias = &gamma,
    },
    [1] = {
        .row[1].slot = &gamma,
        .row[1].fn = add_three,
        .row[1] = {
            .slot = &alpha,
            .fn = mul_three,
        },
        .alias = &beta,
    },
};

static unsigned checksum(void) {
    unsigned acc = 0;

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            const struct Node *node = &tables[i].row[j];
            int slot = node->slot ? *node->slot : 0;
            int fnv = node->fn ? node->fn(i + j + 2) : 0;
            int alias = tables[i].alias ? *tables[i].alias : 0;
            acc = acc * 43u + (unsigned)(slot * 7 + fnv * 5 + alias);
        }
    }
    return acc;
}

int main(void) {
    printf("%d %d %d %u\n",
           tables[0].row[0].fn ? tables[0].row[0].fn(2) : 0,
           tables[1].row[1].slot ? *tables[1].row[1].slot : 0,
           tables[1].alias ? *tables[1].alias : 0,
           checksum());
    return 0;
}
