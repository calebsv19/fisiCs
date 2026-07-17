#include <stddef.h>
#include <stdio.h>

union Payload {
    unsigned char bytes[5];
    unsigned int word;
};

struct Cell {
    unsigned char tag;
    union Payload payload;
};

struct Table {
    struct Cell cells[2][2];
    unsigned char seal;
};

static struct Table table = (struct Table){
    .cells[0][0] = (struct Cell){ .tag = 3, .payload.bytes = { 5, 7, 11, 13 } },
    .cells[0][1].payload = (union Payload){ .bytes = { [1] = 17, [4] = 19 } },
    .cells[0][1].tag = 23,
    .cells[1] = {
        [0] = (struct Cell){ .tag = 29, .payload.bytes = { [2] = 31 } },
    },
    .seal = 37,
};

static unsigned checksum(void) {
    unsigned acc = table.seal;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const struct Cell *cell = &table.cells[r][c];
            acc = acc * 43u + cell->tag;
            acc = acc * 41u + cell->payload.bytes[0];
            acc = acc * 37u + cell->payload.bytes[2];
            acc = acc * 31u + cell->payload.bytes[4];
        }
    }

    return acc;
}

int main(void) {
    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Table, seal),
           (unsigned)table.cells[0][1].payload.bytes[0],
           (unsigned)table.cells[0][1].payload.bytes[4],
           (unsigned)table.cells[1][0].payload.bytes[2],
           (unsigned)table.cells[1][1].tag,
           checksum());
    return 0;
}
