#include <stddef.h>
#include <stdio.h>

union Payload {
    unsigned char bytes[8];
    unsigned short words[4];
    struct {
        unsigned char lo;
        unsigned char hi;
        unsigned short code;
    } tagged;
};

struct Cell {
    unsigned char id;
    union Payload payload;
    unsigned char flags[3];
};

struct Grid {
    struct Cell cells[2][2];
    union Payload footer;
};

static unsigned checksum(const struct Grid *grid) {
    unsigned acc = grid->footer.bytes[0] + grid->footer.bytes[7];

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const struct Cell *cell = &grid->cells[r][c];
            acc = acc * 37u + cell->id;
            acc = acc * 31u + cell->payload.bytes[0];
            acc = acc * 29u + cell->payload.bytes[3];
            acc = acc * 23u + cell->payload.bytes[7];
            acc = acc * 17u + cell->flags[0];
            acc = acc * 13u + cell->flags[2];
        }
    }

    return acc;
}

int main(void) {
    struct Grid grid = {
        .cells[0][0].payload.bytes = { 1, 2, 3 },
        .cells[0][0].flags[2] = 5,
        .cells[0][1] = {
            .id = 7,
            .payload.tagged = { .lo = 11, .hi = 13, .code = 17 },
            .flags = { 19 },
        },
        .cells[1][0].payload.words[3] = 23,
        .cells[1][1].id = 29,
        .cells[1][1].payload.bytes[7] = 31,
        .footer.bytes = { [2] = 37, [7] = 41 },
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Grid, cells),
           (unsigned)offsetof(struct Cell, payload),
           (unsigned)offsetof(struct Cell, flags),
           (unsigned)grid.cells[0][0].payload.bytes[4],
           (unsigned)grid.cells[0][1].flags[1],
           (unsigned)grid.cells[1][0].payload.bytes[0],
           checksum(&grid));
    return 0;
}
