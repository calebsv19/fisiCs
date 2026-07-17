#include <stdio.h>

typedef union SlotPayload {
    unsigned char byte[5];
    struct {
        unsigned char a;
        unsigned char b;
        unsigned char c;
        unsigned char d;
        unsigned char e;
    } named;
} SlotPayload;

typedef struct Slot {
    unsigned short id;
    SlotPayload payload;
} Slot;

typedef struct Packet {
    Slot slot[4];
    unsigned short guard;
} Packet;

typedef Packet (*PacketStep)(Packet, unsigned char);

static Packet seed_packet(unsigned seed) {
    Packet out;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 4u; ++i) {
        out.slot[i].id = (unsigned short)(0x4100u + seed + i * 0x21u);
        for (k = 0u; k < 5u; ++k) {
            out.slot[i].payload.byte[k] = (unsigned char)(0x19u + seed * 3u + i * 13u + k * 5u);
        }
    }
    out.guard = (unsigned short)(0x7300u + seed * 17u);
    return out;
}

static Packet step_left(Packet in, unsigned char salt) {
    Packet out = in;
    Slot hold = out.slot[3];

    out.slot[3] = out.slot[0];
    out.slot[0] = out.slot[1];
    out.slot[1] = hold;
    out.slot[2].payload.named.c = (unsigned char)(out.slot[2].payload.named.c + salt);
    out.guard = (unsigned short)(out.guard ^ (unsigned short)(salt * 29u));
    return out;
}

static Packet step_right(Packet in, unsigned char salt) {
    Packet out = in;

    out.slot[0].payload.byte[4] = (unsigned char)(out.slot[0].payload.byte[4] ^ out.slot[2].payload.byte[1]);
    out.slot[2].id = (unsigned short)(out.slot[2].id + out.slot[3].payload.named.e + salt);
    out.slot[3].payload.named.a = (unsigned char)(out.slot[3].payload.named.a + out.slot[1].payload.byte[0]);
    out.guard = (unsigned short)(out.guard + out.slot[2].id + out.slot[0].payload.byte[4]);
    return out;
}

static PacketStep select_step(Packet value, unsigned round) {
    PacketStep steps[2];

    steps[0] = step_left;
    steps[1] = step_right;
    return steps[(value.slot[round & 3u].payload.byte[round % 5u] + round) & 1u];
}

static unsigned fold_packet(Packet value) {
    unsigned acc = (unsigned)value.guard * 173u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 4u; ++i) {
        acc = acc * 109u + (unsigned)value.slot[i].id;
        for (k = 0u; k < 5u; ++k) {
            acc = acc * 109u + (unsigned)value.slot[i].payload.byte[k];
        }
    }
    return acc;
}

int main(void) {
    Packet packet = seed_packet(9u);
    unsigned i;
    unsigned typed = 0u;

    for (i = 0u; i < 5u; ++i) {
        PacketStep step = select_step(packet, i);
        packet = step(packet, (unsigned char)(0x11u + i * 7u));
    }

    typed += (unsigned)packet.guard;
    typed += (unsigned)packet.slot[0].id * 3u + (unsigned)packet.slot[0].payload.byte[4] * 5u;
    typed += (unsigned)packet.slot[2].id * 7u + (unsigned)packet.slot[3].payload.named.a * 11u;

    printf("%u %u %u %u %u %u %u %u\n",
           (unsigned)packet.slot[0].id,
           (unsigned)packet.slot[0].payload.byte[4],
           (unsigned)packet.slot[1].payload.named.c,
           (unsigned)packet.slot[2].id,
           (unsigned)packet.slot[3].payload.named.a,
           (unsigned)packet.guard,
           typed,
           fold_packet(packet));
    return 0;
}
