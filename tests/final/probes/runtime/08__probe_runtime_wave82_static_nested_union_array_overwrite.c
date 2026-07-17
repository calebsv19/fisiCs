#include <stddef.h>
#include <stdio.h>

union Payload {
    unsigned char bytes[4];
    struct {
        unsigned char left;
        unsigned char right;
        unsigned char aux;
        unsigned char mark;
    } fields;
};

struct Cell {
    unsigned char tag;
    union Payload payload;
};

struct Board {
    struct Cell cells[2][2];
    unsigned char seal;
};

static const struct Board board = {
    .cells[0][0].payload.fields = { .left = 3, .right = 5, .mark = 7 },
    .cells[0][0] = { .tag = 11, .payload.bytes = { [1] = 13, [3] = 17 } },
    .cells[0][1] = { .tag = 19, .payload.fields = { .left = 23, .aux = 29 } },
    .cells[1] = {
        [0] = { .tag = 31, .payload.bytes = { [0] = 37, [2] = 41 } },
    },
    .cells[1][0].payload = { .fields = { .right = 43, .mark = 47 } },
    .seal = 53,
};

int main(void) {
    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Board, seal),
           (unsigned)board.cells[0][0].tag,
           (unsigned)board.cells[0][0].payload.bytes[1],
           (unsigned)board.cells[0][1].payload.bytes[2],
           (unsigned)board.cells[1][0].payload.bytes[3],
           (unsigned)board.cells[1][1].payload.bytes[0]);
    return 0;
}
