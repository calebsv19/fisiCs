#include <stdio.h>

typedef struct Cell {
    unsigned short id;
    union {
        struct {
            unsigned char lo;
            unsigned char mid;
            unsigned char hi;
        };
        unsigned char raw[3];
    } payload;
} Cell;

typedef struct Packet {
    Cell slots[4];
    unsigned short checksum;
} Packet;

typedef Packet (*PacketOp)(Packet, unsigned char);

static Cell make_cell(unsigned char seed, unsigned short id_base) {
    Cell out;

    out.id = (unsigned short)(id_base + seed * 5u);
    out.payload.raw[0] = (unsigned char)(0x21u + seed);
    out.payload.raw[1] = (unsigned char)(0x41u + seed * 3u);
    out.payload.raw[2] = (unsigned char)(0x61u + seed * 7u);
    return out;
}

static Packet seed_packet(unsigned char seed) {
    Packet out;
    unsigned i;

    for (i = 0u; i < 4u; ++i) {
        out.slots[i] = make_cell((unsigned char)(seed + i * 4u), (unsigned short)(0x5100u + i * 0x31u));
    }
    out.checksum = (unsigned short)(0x7300u + seed * 13u);
    return out;
}

static Packet rotate_packet(Packet in, unsigned char salt) {
    Packet out = in;
    Cell save = out.slots[3];

    out.slots[3] = out.slots[1];
    out.slots[1] = out.slots[0];
    out.slots[0] = save;
    out.slots[2].payload.mid = (unsigned char)(out.slots[2].payload.mid ^ salt);
    out.checksum = (unsigned short)(out.checksum + out.slots[0].payload.hi + out.slots[2].id);
    return out;
}

static Packet blend_packet(Packet in, unsigned char salt) {
    Packet out = in;

    out.slots[0].payload.lo = (unsigned char)(out.slots[0].payload.lo + out.slots[3].payload.raw[1]);
    out.slots[1].payload.raw[2] = (unsigned char)(out.slots[1].payload.raw[2] ^ (salt + out.slots[2].payload.lo));
    out.slots[3].id = (unsigned short)(out.slots[3].id + out.slots[1].payload.hi);
    out.checksum = (unsigned short)(out.checksum ^ (unsigned short)(salt * 0x25u));
    return out;
}

static PacketOp choose(unsigned selector) {
    PacketOp table[2];

    table[0] = rotate_packet;
    table[1] = blend_packet;
    return table[selector & 1u];
}

static unsigned fold_packet(Packet value) {
    unsigned acc = (unsigned)value.checksum * 181u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 4u; ++i) {
        acc = acc * 137u + (unsigned)value.slots[i].id;
        for (k = 0u; k < 3u; ++k) {
            acc = acc * 101u + (unsigned)value.slots[i].payload.raw[k] + k;
        }
    }
    return acc;
}

int main(void) {
    Packet packet = seed_packet(6u);
    unsigned i;
    unsigned typed = 0u;

    for (i = 0u; i < 6u; ++i) {
        PacketOp op = choose(i + packet.slots[i & 3u].payload.raw[0]);
        packet = op(packet, (unsigned char)(0x14u + i * 9u));
    }

    typed += (unsigned)packet.checksum;
    typed += (unsigned)packet.slots[0].id * 3u + (unsigned)packet.slots[0].payload.lo * 5u;
    typed += (unsigned)packet.slots[1].payload.raw[2] * 7u + (unsigned)packet.slots[3].id * 11u;

    printf("%u %u %u %u %u %u %u %u\n",
           (unsigned)packet.slots[0].id,
           (unsigned)packet.slots[0].payload.raw[0],
           (unsigned)packet.slots[1].payload.raw[2],
           (unsigned)packet.slots[2].payload.raw[1],
           (unsigned)packet.slots[3].id,
           (unsigned)packet.checksum,
           typed,
           fold_packet(packet));
    return 0;
}
