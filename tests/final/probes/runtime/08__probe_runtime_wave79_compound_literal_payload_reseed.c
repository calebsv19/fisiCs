#include <stddef.h>
#include <stdio.h>

union Payload {
    unsigned char bytes[4];
    unsigned int word;
};

struct Cell {
    unsigned char tag;
    union Payload payload;
};

struct Frame {
    struct Cell cells[3];
    unsigned char tail;
};

static unsigned checksum(const struct Frame *frame) {
    unsigned acc = frame->tail;

    for (int i = 0; i < 3; ++i) {
        const struct Cell *cell = &frame->cells[i];
        acc = acc * 41u + cell->tag;
        acc = acc * 31u + cell->payload.bytes[0];
        acc = acc * 29u + cell->payload.bytes[2];
        acc = acc * 23u + cell->payload.bytes[3];
    }

    return acc;
}

int main(void) {
    struct Frame frame = {
        .cells[0] = { .tag = 3, .payload.bytes = { 5, 7, 11, 13 } },
        .cells[1].payload.bytes[2] = 17,
        .tail = 19,
    };

    frame.cells[1] = (struct Cell){ .tag = 23, .payload.bytes = { 29, 31 } };
    frame.cells[2].payload = (union Payload){ .bytes = { [1] = 37, [3] = 41 } };
    frame.cells[2].tag = 43;

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Frame, tail),
           (unsigned)frame.cells[1].payload.bytes[2],
           (unsigned)frame.cells[2].payload.bytes[0],
           (unsigned)frame.cells[2].payload.bytes[1],
           (unsigned)frame.cells[2].payload.bytes[3],
           checksum(&frame));
    return 0;
}
