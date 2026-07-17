#include <stdio.h>

typedef union {
    unsigned int word;
    unsigned short half[2];
    unsigned char bytes[4];
} Wave52View;

typedef struct {
    Wave52View view;
    unsigned int tag;
} Wave52Packet;

static unsigned int packet_score(Wave52Packet packet) {
    return packet.tag * 17u
        + packet.view.word * 3u
        + (unsigned int)packet.view.half[0] * 5u
        + (unsigned int)packet.view.half[1] * 7u
        + (unsigned int)packet.view.bytes[0] * 11u
        + (unsigned int)packet.view.bytes[3] * 13u;
}

int main(void) {
    Wave52Packet packets[3] = {
        {{0x10203040u}, 3u},
        {{0x55667788u}, 5u},
        {{0xa1b2c3d4u}, 7u}
    };
    unsigned int total = 0;
    int i;

    for (i = 0; i < 9; ++i) {
        Wave52Packet *packet = &packets[i % 3];
        unsigned int low = (unsigned int)packet->view.half[i & 1];
        unsigned int byte = (unsigned int)packet->view.bytes[(i + 1) & 3];

        packet->view.word ^= (low << ((i & 1) * 4)) + byte + (unsigned int)i;
        packet->tag += (packet->view.word >> ((i & 3) * 4)) & 31u;
        total += packet_score(*packet);
    }

    printf("%u %u %u %u\n", packets[0].view.word, packets[1].tag,
           packets[2].view.half[1], total);
    return 0;
}
