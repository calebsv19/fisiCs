#include <stddef.h>
#include <stdio.h>

struct Lane {
    unsigned char tag;
    union {
        struct {
            unsigned char a;
            unsigned char b;
            unsigned char c;
        };
        unsigned char raw[3];
    };
    unsigned char mark;
};

struct Packet {
    struct Lane lanes[3];
    unsigned char tail;
};

static unsigned checksum(const struct Packet *packet) {
    unsigned acc = packet->tail;

    for (int i = 0; i < 3; ++i) {
        const struct Lane *lane = &packet->lanes[i];
        acc = acc * 41u + lane->tag;
        acc = acc * 23u + lane->raw[0];
        acc = acc * 17u + lane->raw[2];
        acc = acc * 13u + lane->mark;
    }

    return acc;
}

int main(void) {
    struct Packet packet = {
        .lanes[0] = { .tag = 2, .a = 3, .b = 5, .mark = 7 },
        .lanes[1].raw = { 11, 13, 17 },
        .lanes[2] = { .tag = 19, .c = 23 },
        .tail = 29,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Packet, lanes),
           (unsigned)offsetof(struct Lane, raw),
           (unsigned)offsetof(struct Packet, tail),
           (unsigned)packet.lanes[0].raw[2],
           (unsigned)packet.lanes[2].raw[0],
           checksum(&packet));
    return 0;
}
