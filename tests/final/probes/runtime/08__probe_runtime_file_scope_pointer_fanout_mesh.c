#include <stdio.h>

static int alpha = 2;
static int beta = 4;
static int gamma = 8;

static int plus_five(int x) {
    return x + 5;
}

static int times_three(int x) {
    return x * 3;
}

struct Leaf {
    int *slot;
    int (*fn)(int);
};

struct Group {
    struct Leaf leaves[2];
    int *aliases[2];
};

static struct Group groups[2] = {
    [0] = {
        .leaves[0].slot = &alpha,
        .leaves[0].fn = plus_five,
        .aliases[1] = &gamma,
    },
    [1] = {
        .leaves[1] = {
            .slot = &beta,
            .fn = times_three,
        },
        .aliases[0] = &alpha,
        .aliases[1] = &beta,
    },
};

static unsigned checksum(void) {
    unsigned acc = 0;

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            int slot = groups[i].leaves[j].slot ? *groups[i].leaves[j].slot : 0;
            int fnv = groups[i].leaves[j].fn ? groups[i].leaves[j].fn(j + 2) : 0;
            int alias = groups[i].aliases[j] ? *groups[i].aliases[j] : 0;
            acc = acc * 31u + (unsigned)(slot * 7 + fnv * 5 + alias);
        }
    }
    return acc;
}

int main(void) {
    printf("%d %d %d %u\n",
           groups[0].leaves[0].fn ? groups[0].leaves[0].fn(3) : 0,
           groups[0].aliases[1] ? *groups[0].aliases[1] : 0,
           groups[1].leaves[1].slot ? *groups[1].leaves[1].slot : 0,
           checksum());
    return 0;
}
