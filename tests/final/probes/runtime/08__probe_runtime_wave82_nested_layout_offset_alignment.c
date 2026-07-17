#include <stddef.h>
#include <stdio.h>

union Payload {
    unsigned int word;
    unsigned char bytes[4];
};

struct Header {
    unsigned char tag;
    union Payload payload;
};

struct Packet {
    unsigned char lead;
    struct Header headers[2];
    unsigned short tail;
};

static const struct Packet packet = {
    .lead = 3,
    .headers[0].payload.bytes = { 5, 7, 11, 13 },
    .headers[0].tag = 17,
    .headers[1].payload.word = 19,
    .headers[1].tag = 23,
    .tail = 29,
};

int main(void) {
    printf("%u %u %u %u %u %u\n",
           (unsigned)offsetof(struct Packet, headers),
           (unsigned)offsetof(struct Packet, tail),
           (unsigned)offsetof(struct Header, payload),
           (unsigned)sizeof(struct Header),
           (unsigned)_Alignof(struct Packet),
           (unsigned)packet.headers[0].payload.bytes[2]);
    return 0;
}
