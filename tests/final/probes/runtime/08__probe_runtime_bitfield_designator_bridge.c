#include <stdio.h>

struct Flags {
    unsigned low : 3;
    unsigned mid : 5;
    unsigned high : 6;
};

struct Packet {
    unsigned head;
    struct Flags flags;
    unsigned tail;
};

int main(void) {
    struct Packet packet = {
        .tail = 9,
        .flags = {
            .mid = 17,
            .high = 33,
        },
        .head = 4,
    };

    printf(
        "%u %u %u %u %u\n",
        packet.head,
        packet.flags.low,
        packet.flags.mid,
        packet.flags.high,
        packet.tail
    );
    return 0;
}
