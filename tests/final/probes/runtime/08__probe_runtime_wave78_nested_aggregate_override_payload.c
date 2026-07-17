#include <stddef.h>
#include <stdio.h>

struct Cell {
    unsigned char tag;
    unsigned char bytes[3];
};

struct Row {
    struct Cell cells[2];
    unsigned char seal;
};

struct Packet {
    struct Row rows[2];
    unsigned char tail;
};

static unsigned checksum(const struct Packet *packet) {
    unsigned acc = packet->tail;

    for (int r = 0; r < 2; ++r) {
        acc = acc * 43u + packet->rows[r].seal;
        for (int c = 0; c < 2; ++c) {
            const struct Cell *cell = &packet->rows[r].cells[c];
            acc = acc * 31u + cell->tag;
            acc = acc * 19u + cell->bytes[0];
            acc = acc * 17u + cell->bytes[2];
        }
    }

    return acc;
}

int main(void) {
    struct Packet packet = {
        .rows[0] = {
            .cells = {
                [0] = { .tag = 3, .bytes = { 5, 7, 11 } },
                [1].bytes[1] = 13,
            },
            .seal = 17,
        },
        .rows[0].cells[1] = { .tag = 19, .bytes = { 23, 29, 31 } },
        .rows[1].cells[0].bytes[2] = 37,
        .tail = 41,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Packet, rows),
           (unsigned)offsetof(struct Row, seal),
           (unsigned)packet.rows[0].cells[1].bytes[1],
           (unsigned)packet.rows[1].cells[0].tag,
           (unsigned)packet.rows[1].cells[0].bytes[2],
           checksum(&packet));
    return 0;
}
