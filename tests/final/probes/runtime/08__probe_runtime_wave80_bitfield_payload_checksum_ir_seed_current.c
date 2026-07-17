#include <stddef.h>
#include <stdio.h>

struct Flags {
    unsigned int lo : 3;
    unsigned int mid : 5;
    unsigned int hi : 6;
};

union Payload {
    unsigned char bytes[4];
    struct Flags flags;
};

struct Item {
    unsigned char lane;
    union Payload payload;
};

struct Packet {
    struct Item items[3];
    unsigned char end;
};

static struct Item choose_item(int selector) {
    return selector ? (struct Item){ .lane = 7, .payload.flags = { .lo = 5, .mid = 17, .hi = 33 } }
                    : (struct Item){ .lane = 11, .payload.bytes = { [0] = 13, [3] = 19 } };
}

static unsigned checksum(const struct Packet *packet) {
    unsigned acc = packet->end;

    for (int i = 0; i < 3; ++i) {
        const struct Item *item = &packet->items[i];
        acc = acc * 59u + item->lane;
        acc = acc * 53u + item->payload.flags.lo;
        acc = acc * 47u + item->payload.flags.mid;
        acc = acc * 43u + item->payload.flags.hi;
    }

    return acc;
}

int main(void) {
    struct Packet packet = {
        .items[0] = choose_item(1),
        .items[1].payload.bytes[3] = 23,
        .items[2] = { .lane = 29, .payload.flags = { .lo = 3, .mid = 21, .hi = 45 } },
        .end = 31,
    };

    packet.items[1] = (struct Item){ .lane = 37, .payload = (union Payload){ .bytes = { [0] = 41, [2] = 43 } } };
    packet.items[2] = choose_item(0);

    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Packet, end),
           (unsigned)packet.items[0].payload.flags.mid,
           (unsigned)packet.items[1].payload.bytes[2],
           (unsigned)packet.items[1].payload.bytes[3],
           (unsigned)packet.items[2].payload.bytes[3],
           checksum(&packet));
    return 0;
}
