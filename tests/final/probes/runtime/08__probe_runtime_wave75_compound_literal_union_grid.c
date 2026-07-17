#include <stddef.h>
#include <stdio.h>

union Cell {
    unsigned char bytes[5];
    struct {
        unsigned char tag;
        unsigned char value;
        unsigned char pad[3];
    } meta;
};

struct Row {
    union Cell cells[3];
    unsigned char mark;
};

struct Grid {
    struct Row rows[2];
    unsigned char tail;
};

static unsigned checksum(const struct Grid *grid) {
    unsigned acc = grid->tail;

    for (int r = 0; r < 2; ++r) {
        const struct Row *row = &grid->rows[r];
        acc = acc * 41u + row->mark;
        for (int c = 0; c < 3; ++c) {
            const union Cell *cell = &row->cells[c];
            acc = acc * 29u + cell->bytes[0];
            acc = acc * 23u + cell->bytes[1];
            acc = acc * 17u + cell->bytes[4];
        }
    }

    return acc;
}

int main(void) {
    struct Grid grid = {
        .rows[0].cells[1].meta = { .tag = 3, .value = 5, .pad = { [2] = 7 } },
        .rows[0].cells[2].bytes = { 11, 13 },
        .rows[0].mark = 17,
        .rows[1] = {
            .cells = {
                [0].bytes = { 19, 23, 29 },
                [2].meta = { .tag = 31, .value = 37 },
            },
            .mark = 41,
        },
        .rows[1].cells[2].bytes[4] = 43,
        .tail = 47,
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Grid, rows),
           (unsigned)offsetof(struct Row, cells),
           (unsigned)offsetof(struct Row, mark),
           (unsigned)grid.rows[0].cells[0].bytes[0],
           (unsigned)grid.rows[0].cells[1].bytes[2],
           (unsigned)grid.rows[1].cells[2].bytes[4],
           checksum(&grid));
    return 0;
}
