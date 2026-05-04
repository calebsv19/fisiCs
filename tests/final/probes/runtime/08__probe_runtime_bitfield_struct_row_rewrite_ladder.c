#include <stdio.h>

struct Bits {
    unsigned a : 2;
    unsigned b : 4;
    unsigned c : 3;
    unsigned d : 5;
};

struct RowCell {
    struct Bits bits;
    unsigned extra;
};

struct Ladder {
    struct RowCell rows[2][3];
    unsigned tail;
};

static unsigned checksum(const struct Ladder ladders[2]) {
    unsigned acc = 0;

    for (int i = 0; i < 2; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 3; ++c) {
                const struct RowCell *cell = &ladders[i].rows[r][c];
                acc = acc * 79u +
                      cell->bits.a * 17u +
                      cell->bits.b * 11u +
                      cell->bits.c * 7u +
                      cell->bits.d * 5u +
                      cell->extra * 3u +
                      ladders[i].tail;
            }
        }
    }
    return acc;
}

int main(void) {
    struct Ladder ladders[2] = {
        [0] = {
            .rows[0] = {
                [0] = { .bits = { .a = 1, .b = 3 }, .extra = 2 },
                [1] = { .bits = { .c = 4, .d = 7 }, .extra = 5 },
                [2] = { .bits = { .b = 8 }, .extra = 6 },
            },
            .rows[1][1] = { .bits = { .a = 2, .d = 9 }, .extra = 4 },
            .tail = 3,
        },
        [1] = {
            .rows[1] = {
                [0] = { .bits = { .a = 3, .b = 5 }, .extra = 1 },
                [1] = { .bits = { .c = 6, .d = 10 }, .extra = 7 },
                [2] = { .bits = { .b = 9 }, .extra = 8 },
            },
            .rows[0][2] = { .bits = { .a = 1, .c = 2 }, .extra = 9 },
            .tail = 5,
        },
    };

    printf("%u %u %u %u\n",
           ladders[0].rows[0][1].bits.d,
           ladders[0].rows[1][1].extra,
           ladders[1].rows[1][2].bits.b,
           checksum(ladders));
    return 0;
}
