#include <stdio.h>

union Payload {
    char text[4];
    struct {
        unsigned char a;
        unsigned char b;
        unsigned char c;
        unsigned char d;
    } bytes;
};

struct Flags {
    unsigned lo : 3;
    unsigned mid : 5;
    unsigned hi : 4;
};

struct Cell {
    union Payload payload;
    struct Flags flags;
    unsigned mark;
};

struct Grid {
    struct Cell cells[2][2];
    unsigned tail;
};

static unsigned checksum(const struct Grid grids[2]) {
    unsigned acc = 0;

    for (int g = 0; g < 2; ++g) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Cell *cell = &grids[g].cells[r][c];
                acc = acc * 43u +
                      cell->payload.bytes.a * 11u +
                      cell->payload.bytes.b * 7u +
                      cell->payload.bytes.c * 5u +
                      cell->payload.bytes.d * 3u +
                      cell->flags.lo * 17u +
                      cell->flags.mid * 13u +
                      cell->flags.hi * 19u +
                      cell->mark * 23u +
                      grids[g].tail;
            }
        }
    }

    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .cells[0][1].payload = {
                .text = { 'a', 'b', 'Z', 0 },
            },
            .cells[0][1].flags.lo = 5,
            .cells[0][1].flags.hi = 9,
            .cells[0][1].mark = 3,
            .cells[1][0].payload = {
                .text = { 'q', 'r', 0, 0 },
            },
            .cells[1][0].flags.mid = 12,
            .cells[1][0].mark = 4,
            .tail = 2,
        },
        [1] = {
            .cells[1][1].payload = {
                .text = { 'k', '!', 0, 0 },
            },
            .cells[1][1].flags.lo = 1,
            .cells[1][1].flags.mid = 17,
            .cells[1][1].flags.hi = 6,
            .cells[1][1].mark = 7,
            .cells[0][0].payload = {
                .text = { 0, 0, 0, 'P' },
            },
            .cells[0][0].flags.hi = 5,
            .cells[0][0].mark = 8,
            .tail = 6,
        },
    };

    printf("%u %u %u %u\n",
           (unsigned)grids[0].cells[0][1].payload.bytes.c,
           grids[1].cells[1][1].flags.mid,
           (unsigned)grids[1].cells[0][0].payload.bytes.d,
           checksum(grids));
    return 0;
}
