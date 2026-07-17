#include <stdio.h>

typedef struct Leaf {
    union {
        struct {
            unsigned char lo;
            unsigned char hi;
        };
        unsigned short half;
        unsigned char raw[2];
    };
} Leaf;

typedef struct Packet {
    unsigned char head;
    Leaf lanes[3];
    unsigned char tail;
} Packet;

static unsigned byte_hash_packet(Packet value) {
    unsigned hash = 2166136261u;
    unsigned i;

    hash ^= (unsigned)value.head;
    hash *= 16777619u;
    for (i = 0u; i < 3u; ++i) {
        hash ^= (unsigned)value.lanes[i].raw[0];
        hash *= 16777619u;
        hash ^= (unsigned)value.lanes[i].raw[1];
        hash *= 16777619u;
    }
    hash ^= (unsigned)value.tail;
    hash *= 16777619u;
    return hash;
}

static Packet build_packet(unsigned char seed) {
    Packet out;
    out.head = (unsigned char)(0x40u + seed);
    out.lanes[0].lo = (unsigned char)(0x11u + seed);
    out.lanes[0].hi = (unsigned char)(0x22u + seed);
    out.lanes[1].raw[0] = (unsigned char)(0x33u + seed);
    out.lanes[1].raw[1] = (unsigned char)(0x44u + seed);
    out.lanes[2].half = (unsigned short)(0x5566u + seed);
    out.tail = (unsigned char)(0x70u + seed);
    return out;
}

static Packet route_packet(Packet in) {
    Packet out = in;
    out.lanes[0] = in.lanes[2];
    out.lanes[1].raw[0] ^= 0x5Au;
    out.lanes[1].raw[1] ^= 0xA5u;
    out.lanes[2] = in.lanes[0];
    out.tail = (unsigned char)(out.tail + out.lanes[0].lo + out.lanes[2].hi);
    return out;
}

int main(void) {
    Packet a = build_packet(7u);
    Packet b = route_packet(a);
    unsigned typed = 0u;

    typed += (unsigned)b.head * 3u;
    typed += (unsigned)b.lanes[0].lo * 5u;
    typed += (unsigned)b.lanes[0].hi * 7u;
    typed += (unsigned)b.lanes[1].lo * 11u;
    typed += (unsigned)b.lanes[1].hi * 13u;
    typed += (unsigned)b.lanes[2].raw[0] * 17u;
    typed += (unsigned)b.lanes[2].raw[1] * 19u;
    typed += (unsigned)b.tail * 23u;

    printf("%u %u %u %u %u %u %u %u %u\n",
           (unsigned)b.head,
           (unsigned)b.lanes[0].lo,
           (unsigned)b.lanes[0].hi,
           (unsigned)b.lanes[1].lo,
           (unsigned)b.lanes[1].hi,
           (unsigned)b.lanes[2].lo,
           (unsigned)b.lanes[2].hi,
           typed,
           byte_hash_packet(b));
    return 0;
}
