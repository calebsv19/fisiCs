#include <stddef.h>
#include <stdio.h>

struct Cell {
    unsigned char tag;
    unsigned char lane[3];
};

union Payload {
    struct Cell cell;
    unsigned char raw[4];
};

struct Row {
    union Payload payloads[3];
};

struct Grid {
    struct Row rows[2];
    unsigned char guard;
};

static unsigned checksum(const struct Grid *grid) {
    unsigned acc = grid->guard;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 3; ++c) {
            const union Payload *payload = &grid->rows[r].payloads[c];
            acc = acc * 37u + payload->raw[0];
            acc = acc * 29u + payload->raw[1];
            acc = acc * 23u + payload->raw[3];
        }
    }

    return acc;
}

int main(void) {
    struct Grid grid = {
        .rows[0] = (struct Row){
            .payloads = {
                [0].cell = { .tag = 3, .lane = { 5, 7 } },
                [2].raw = { 11, 13, 17, 19 },
            },
        },
        .rows[1].payloads[1] = (union Payload){ .cell = { .tag = 23, .lane = { [2] = 29 } } },
        .rows[1].payloads[2].raw = { 31, 37 },
        .guard = 41,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Grid, rows),
           (unsigned)sizeof(grid.rows[0].payloads[0]),
           (unsigned)grid.rows[0].payloads[1].raw[0],
           (unsigned)grid.rows[1].payloads[1].raw[3],
           (unsigned)grid.rows[1].payloads[2].raw[2],
           checksum(&grid));
    return 0;
}
