#include <stddef.h>
#include <stdio.h>

struct Slot {
    unsigned char tag;
    unsigned char lanes[3];
};

union Cell {
    struct Slot slot;
    unsigned char raw[4];
};

struct Board {
    union Cell cells[2][3];
    unsigned char guard;
};

static unsigned checksum(const struct Board *board) {
    unsigned acc = board->guard;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 3; ++c) {
            const union Cell *cell = &board->cells[r][c];
            acc = acc * 43u + cell->raw[0];
            acc = acc * 31u + cell->raw[2];
            acc = acc * 17u + cell->raw[3];
        }
    }

    return acc;
}

int main(void) {
    struct Board board = {
        .cells[0] = {
            [0].slot = { .tag = 3, .lanes = { 5, 7 } },
            [2].raw = { 11, 13 },
        },
        .cells[0][0].raw[2] = 17,
        .cells[1][1].slot.lanes[1] = 19,
        .cells[1][2].slot = { .tag = 23, .lanes = { [2] = 29 } },
        .guard = 31,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Board, cells),
           (unsigned)offsetof(struct Board, guard),
           (unsigned)board.cells[0][0].raw[2],
           (unsigned)board.cells[0][2].raw[2],
           (unsigned)board.cells[1][0].raw[0],
           checksum(&board));
    return 0;
}
