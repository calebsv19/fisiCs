#include <stdio.h>

enum Scale {
    SCALE_NEG = -2,
    SCALE_BASE = 4,
    SCALE_HIGH = 9
};

struct Inner {
    unsigned char raw;
    signed char delta;
};

union Slot {
    struct Inner inner;
    unsigned short packed;
};

struct Packet {
    enum Scale scale;
    union Slot slot;
    long bias;
};

static int fold_packet(struct Packet *packet, int add) {
    unsigned int raw = packet->slot.inner.raw;
    int delta = packet->slot.inner.delta;
    long mixed = (long)(packet->scale + delta) + packet->bias;
    return (int)(raw + (unsigned int)add) + (int)mixed;
}

int main(void) {
    struct Packet packets[2] = {
        {SCALE_BASE, {{250u, -6}}, 11},
        {SCALE_NEG, {{12u, 7}}, -3}
    };

    struct Packet *selected = 1 ? &packets[0] : &packets[1];
    int first = fold_packet(selected, packets[1].slot.inner.raw);
    int second = fold_packet(0 ? &packets[0] : &packets[1], (unsigned char)245u);
    unsigned int compare = (unsigned int)packets[0].slot.inner.raw > (unsigned int)packets[1].slot.inner.delta;

    printf("%d %d %u %ld\n",
           first,
           second,
           compare,
           (long)(selected->slot.inner.delta + selected->scale));
    return 0;
}
