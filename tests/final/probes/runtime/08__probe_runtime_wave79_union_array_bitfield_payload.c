#include <stddef.h>
#include <stdio.h>

struct Flags {
    unsigned int low : 4;
    unsigned int high : 4;
};

union Payload {
    unsigned char bytes[4];
    struct Flags flags;
};

struct Slot {
    unsigned char kind;
    union Payload payload;
};

struct Packet {
    struct Slot slots[2][2];
    unsigned char done;
};

static unsigned checksum(const struct Packet *packet) {
    unsigned acc = packet->done;

    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            const struct Slot *slot = &packet->slots[r][c];
            acc = acc * 47u + slot->kind;
            if (slot->kind == 1) {
                acc = acc * 41u + slot->payload.bytes[0];
                acc = acc * 37u + slot->payload.bytes[3];
            } else {
                acc = acc * 31u + slot->payload.flags.low;
                acc = acc * 29u + slot->payload.flags.high;
            }
        }
    }

    return acc;
}

int main(void) {
    struct Packet packet = {
        .slots[0][0] = { .kind = 1, .payload.bytes = "q" },
        .slots[0][1].payload.flags = { .low = 7, .high = 9 },
        .slots[0][1].kind = 2,
        .slots[1][0] = { .kind = 1, .payload.bytes = { 'a', 'b', 0, 'd' } },
        .slots[1][1].payload.bytes[2] = 'z',
        .done = 13,
    };

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Packet, done),
           (unsigned)packet.slots[0][0].payload.bytes[1],
           (unsigned)packet.slots[0][1].payload.flags.low,
           (unsigned)packet.slots[0][1].payload.flags.high,
           (unsigned)packet.slots[1][1].payload.bytes[2],
           checksum(&packet));
    return 0;
}
