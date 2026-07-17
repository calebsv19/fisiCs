#include <stddef.h>
#include <stdio.h>

struct Pair {
    int x;
    int y;
};

union Slot {
    int scalar;
    struct Pair pair;
    unsigned char bytes[8];
};

struct Row {
    union Slot slots[4];
    int bias;
};

struct Board {
    struct Row rows[2];
    union Slot tail[3];
    int mark;
};

static int checksum(const struct Board *board) {
    int acc = board->mark;

    for (int r = 0; r < 2; ++r) {
        acc = acc * 31 + board->rows[r].bias;
        for (int c = 0; c < 4; ++c) {
            acc = acc * 17 + board->rows[r].slots[c].scalar;
        }
    }
    for (int i = 0; i < 3; ++i) {
        acc = acc * 13 + board->tail[i].scalar;
    }

    return acc;
}

int main(void) {
    struct Board board = {
        .rows[0].slots[2].pair = { .x = 3, .y = 5 },
        .rows[0].bias = 7,
        .rows[1].slots[0].scalar = 11,
        .rows[1].slots[3].bytes = { 13, 17, 0, 0, 0, 0, 0, 0 },
        .rows[1].bias = 19,
        .tail[2].scalar = 23,
        .mark = 29,
    };

    printf("%u %u %d %d %d %d\n",
           (unsigned)offsetof(struct Board, rows),
           (unsigned)offsetof(struct Row, slots),
           board.rows[0].slots[0].scalar,
           board.rows[0].slots[2].pair.y,
           board.tail[0].scalar,
           checksum(&board));
    return 0;
}
