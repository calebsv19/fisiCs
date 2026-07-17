#include <stddef.h>
#include <stdio.h>

struct Cell {
    unsigned char tag;
    union {
        struct {
            unsigned short lo;
            unsigned short hi;
        };
        unsigned raw;
        unsigned char bytes[4];
    };
    struct {
        unsigned char mode;
        union {
            unsigned short pair;
            struct {
                unsigned char a;
                unsigned char b;
            };
        };
    };
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
              (unsigned)cell->lo * 17u +
              (unsigned)cell->hi * 13u +
              (unsigned)cell->mode * 11u +
              (unsigned)cell->a * 7u +
              (unsigned)cell->b * 5u;
    }

    return acc;
}

int main(void) {
    struct Grid grid = {
        .cells[0] = {
            .tag = 1,
            .lo = 3,
            .hi = 4,
            .mode = 5,
            .a = 6,
            .b = 7,
        },
        .cells[2] = {
            .tag = 2,
            .raw = 99u,
            .mode = 8,
            .pair = 0x090au,
        },
        .tail = 11u,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Grid, cells),
           (unsigned)offsetof(struct Cell, lo),
           (unsigned)offsetof(struct Cell, raw),
           (unsigned)offsetof(struct Cell, a),
           (unsigned)grid.cells[1].raw,
           checksum(&grid));
    return 0;
}
