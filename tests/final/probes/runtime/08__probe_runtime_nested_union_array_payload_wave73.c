#include <stddef.h>
#include <stdio.h>

struct Pair {
    unsigned short lo;
    unsigned short hi;
};

union Payload {
    struct Pair pair;
    unsigned short shorts[2];
    unsigned char bytes[4];
};

struct Cell {
    unsigned char tag;
    union Payload payload[2];
    unsigned char tail[3];
};

struct Packet {
    struct Cell rows[2][2];
    unsigned char mark;
};

static unsigned checksum(const struct Packet *packet) {
    unsigned acc = packet->mark;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const struct Cell *cell = &packet->rows[r][c];
            acc = acc * 41u + cell->tag;
            for (int p = 0; p < 2; ++p) {
                acc = acc * 17u + cell->payload[p].pair.lo;
                acc = acc * 13u + cell->payload[p].pair.hi;
                acc = acc * 7u + cell->payload[p].bytes[0];
                acc = acc * 5u + cell->payload[p].bytes[3];
            }
            acc = acc * 31u + cell->tail[0];
            acc = acc * 29u + cell->tail[2];
        }
    }

    return acc;
}

int main(void) {
    struct Packet packet = {
        .rows[0][1].tag = 3,
        .rows[0][1].payload[0].pair = { .lo = 5, .hi = 7 },
        .rows[0][1].payload[1].bytes = { 11, 13, 17, 19 },
        .rows[0][1].tail = { 23, 0, 29 },
        .rows[1][0] = {
            .tag = 31,
            .payload = {
                [0].shorts = { 37, 41 },
                [1].pair = { .lo = 43, .hi = 47 },
            },
            .tail[2] = 53,
        },
        .rows[1][1].payload[1].bytes[3] = 59,
        .mark = 61,
    };

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Packet, rows),
           (unsigned)offsetof(struct Cell, payload),
           (unsigned)offsetof(struct Cell, tail),
           (unsigned)packet.rows[0][0].payload[0].pair.lo,
           (unsigned)packet.rows[0][1].payload[1].bytes[2],
           (unsigned)packet.rows[1][0].tail[0],
           checksum(&packet));
    return 0;
}
