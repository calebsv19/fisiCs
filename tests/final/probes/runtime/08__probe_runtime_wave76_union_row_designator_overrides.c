#include <stddef.h>
#include <stdio.h>

struct Pair {
    unsigned char left;
    unsigned char right;
};

union Entry {
    struct Pair pair;
    unsigned char raw[2];
};

struct Table {
    union Entry rows[3][2];
    unsigned char tail;
};

static unsigned checksum(const struct Table *table) {
    unsigned acc = table->tail;

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 2; ++c) {
            acc = acc * 43u + table->rows[r][c].raw[0];
            acc = acc * 17u + table->rows[r][c].raw[1];
        }
    }

    return acc;
}

int main(void) {
    struct Table table = {
        .rows[0][0].pair = { .left = 2, .right = 3 },
        .rows[0][0].raw[1] = 5,
        .rows[1] = {
            [0].raw = { 7, 11 },
            [1].pair = { .left = 13 },
        },
        .rows[1][1].pair.right = 17,
        .rows[2][0] = (union Entry){ .raw = { 19, 23 } },
        .rows[2][0].pair.left = 29,
        .tail = 31,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Table, rows),
           (unsigned)offsetof(struct Table, tail),
           (unsigned)table.rows[0][0].raw[1],
           (unsigned)table.rows[1][1].raw[0],
           (unsigned)table.rows[2][1].raw[0],
           checksum(&table));
    return 0;
}
