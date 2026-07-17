#include <stddef.h>
#include <stdio.h>

struct Pair16 {
    unsigned short lo;
    unsigned short hi;
};

struct BytePair {
    unsigned char a;
    unsigned char b;
};

union Payload {
    struct Pair16 pair;
    unsigned raw;
    unsigned char bytes[4];
};

union ModePayload {
    unsigned short pair;
    struct BytePair bytes;
};

struct Cell {
    unsigned char tag;
    union Payload payload;
    unsigned char mode;
    union ModePayload aux;
};

struct Grid {
    struct Cell cells[3];
    unsigned tail;
};

static unsigned checksum(const struct Grid *grid) {
    unsigned acc = grid->tail;

    for (int i = 0; i < 3; ++i) {
        const struct Cell *cell = &grid->cells[i];
        acc = acc * 37u +
              (unsigned)cell->tag * 19u +
              (unsigned)cell->payload.pair.lo * 17u +
              (unsigned)cell->payload.pair.hi * 13u +
              (unsigned)cell->mode * 11u +
              (unsigned)cell->aux.bytes.a * 7u +
              (unsigned)cell->aux.bytes.b * 5u;
    }

    return acc;
}

int main(void) {
    struct Grid grid = {
        .cells[0] = {
            .tag = 1,
            .payload.pair = { .lo = 3, .hi = 4 },
            .mode = 5,
            .aux.bytes = { .a = 6, .b = 7 },
        },
        .cells[2] = {
            .tag = 2,
            .payload.raw = 99u,
            .mode = 8,
            .aux.pair = 0x090au,
        },
        .tail = 11u,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Grid, cells),
           (unsigned)offsetof(struct Cell, payload),
           (unsigned)offsetof(struct Cell, aux),
           (unsigned)offsetof(struct Grid, tail),
           (unsigned)grid.cells[1].payload.raw,
           checksum(&grid));
    return 0;
}
