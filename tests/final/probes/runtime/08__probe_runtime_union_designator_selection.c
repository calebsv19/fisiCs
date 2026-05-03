#include <stdio.h>

union Payload {
    int scalar;
    unsigned char bytes[4];
};

struct Packet {
    int tag;
    union Payload payload;
    int tail;
};

static struct Packet build_packet(void) {
    struct Packet packet = {
        .tag = 2,
        .payload = {.bytes = {1, 2, 3, 4}},
        .tail = 7,
    };
    return packet;
}

int main(void) {
    struct Packet packet = build_packet();
    printf("%d %u %u %u %u %d\n",
           packet.tag,
           (unsigned)packet.payload.bytes[0],
           (unsigned)packet.payload.bytes[1],
           (unsigned)packet.payload.bytes[2],
           (unsigned)packet.payload.bytes[3],
           packet.tail);
    return 0;
}
