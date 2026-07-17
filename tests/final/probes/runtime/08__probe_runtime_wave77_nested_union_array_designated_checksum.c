#include <stddef.h>
#include <stdio.h>

struct Pair {
    unsigned char left;
    unsigned char right;
};

union Value {
    struct Pair pair;
    unsigned char raw[2];
};

struct Row {
    union Value slots[2][2];
    unsigned char seal;
};

struct Grid {
    struct Row rows[2];
};

static unsigned checksum(const struct Grid *grid) {
    unsigned acc = 0;

    for (int r = 0; r < 2; ++r) {
        acc = acc * 37u + grid->rows[r].seal;
        for (int a = 0; a < 2; ++a) {
            for (int b = 0; b < 2; ++b) {
                const union Value *slot = &grid->rows[r].slots[a][b];
                acc = acc * 31u + slot->raw[0];
                acc = acc * 19u + slot->raw[1];
            }
        }
    }

    return acc;
}

int main(void) {
    struct Grid grid = {
        .rows[0].slots[0][1].pair = { .left = 3, .right = 5 },
        .rows[0].slots[1] = {
            [0].raw = { 7, 11 },
            [1].pair.right = 13,
        },
        .rows[0].seal = 17,
        .rows[1] = {
            .slots = {
                [0][0].pair = { .left = 19 },
                [1][1].raw = { 23, 29 },
            },
            .seal = 31,
        },
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Grid, rows),
           (unsigned)offsetof(struct Row, seal),
           (unsigned)grid.rows[0].slots[0][0].raw[0],
           (unsigned)grid.rows[0].slots[1][1].raw[0],
           (unsigned)grid.rows[1].slots[0][0].raw[1],
           checksum(&grid));
    return 0;
}
