#include <stdio.h>

typedef struct Packet {
    unsigned int key;
    unsigned int payload[2];
} Packet;

static unsigned int outer_checksum(const struct Packet *packet) {
    return packet->key * 3u + packet->payload[0] * 5u +
           packet->payload[1] * 7u;
}

int main(void) {
    Packet outer_typedef = {11u, {17u, 23u}};
    struct Packet outer_tag = {29u, {31u, 37u}};
    unsigned int inner_typedef_sum;
    unsigned int inner_tag_sum;
    unsigned int inner_size;
    unsigned int total;

    {
        typedef struct InnerPacket {
            unsigned int payload[3];
            unsigned int key;
        } InnerPacket;
        InnerPacket inner_typedef = {{5u, 7u, 13u}, 19u};
        struct InnerPacket inner_tag = {{2u, 3u, 11u}, 23u};

        inner_typedef_sum = inner_typedef.payload[0] * 11u +
                            inner_typedef.payload[1] * 13u +
                            inner_typedef.payload[2] * 17u +
                            inner_typedef.key * 19u;
        inner_tag_sum = inner_tag.payload[0] * 11u +
                        inner_tag.payload[1] * 13u +
                        inner_tag.payload[2] * 17u + inner_tag.key * 19u;
        inner_size = (unsigned int)sizeof(InnerPacket);
    }

    total = outer_checksum(&outer_typedef) * 2u +
            outer_checksum(&outer_tag) * 3u + inner_typedef_sum * 5u +
            inner_tag_sum * 7u;
    printf("%u %u %u\n", total, (unsigned int)sizeof(Packet), inner_size);
    return 0;
}
