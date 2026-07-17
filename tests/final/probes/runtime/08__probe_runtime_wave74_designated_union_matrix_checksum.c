#include <stddef.h>
#include <stdio.h>

struct Pair {
    unsigned char a;
    unsigned char b;
};

union Slot {
    struct Pair pair;
    unsigned char bytes[4];
    unsigned short halves[2];
};

struct Row {
    unsigned char tag;
    union Slot slots[3];
    unsigned char tail[2];
};

struct Matrix {
    struct Row rows[2];
    unsigned char guard;
};

static unsigned checksum(const struct Matrix *matrix) {
    unsigned acc = matrix->guard;

    for (int r = 0; r < 2; ++r) {
        const struct Row *row = &matrix->rows[r];
        acc = acc * 43u + row->tag;
        for (int s = 0; s < 3; ++s) {
            acc = acc * 19u + row->slots[s].pair.a;
            acc = acc * 17u + row->slots[s].pair.b;
            acc = acc * 13u + row->slots[s].bytes[2];
            acc = acc * 11u + row->slots[s].bytes[3];
        }
        acc = acc * 7u + row->tail[0];
        acc = acc * 5u + row->tail[1];
    }

    return acc;
}

int main(void) {
    struct Matrix matrix = {
        .rows[0].slots[1].pair = { .a = 3, .b = 5 },
        .rows[0].slots[2].bytes = { 7, 11, 13, 17 },
        .rows[0].tail[1] = 19,
        .rows[1] = {
            .tag = 23,
            .slots = {
                [0].halves = { 29, 31 },
                [2].pair = { .a = 37, .b = 41 },
            },
            .tail = { 43, 0 },
        },
        .rows[1].slots[2].bytes[3] = 47,
        .guard = 53,
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Matrix, rows),
           (unsigned)offsetof(struct Row, slots),
           (unsigned)offsetof(struct Row, tail),
           (unsigned)matrix.rows[0].slots[0].bytes[0],
           (unsigned)matrix.rows[0].slots[1].bytes[2],
           (unsigned)matrix.rows[1].tail[1],
           checksum(&matrix));
    return 0;
}
