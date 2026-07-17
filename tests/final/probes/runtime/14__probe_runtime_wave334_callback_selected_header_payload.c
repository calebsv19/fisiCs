#include <stdio.h>
#include <string.h>

typedef union ChunkPayload {
    unsigned char bytes[7];
    struct {
        unsigned char a;
        unsigned char b;
        unsigned char c;
        unsigned char d;
        unsigned char e;
        unsigned char f;
        unsigned char g;
    } named;
} ChunkPayload;

typedef struct Chunk {
    unsigned short key;
    ChunkPayload payload;
} Chunk;

typedef struct Packet {
    Chunk chunk[4];
    unsigned short guard;
} Packet;

typedef Packet (*PacketStep)(Packet, unsigned char);

static Packet seed_packet(unsigned seed) {
    Packet out;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 4u; ++i) {
        out.chunk[i].key = (unsigned short)(0x2400u + seed + i * 0x31u);
        for (k = 0u; k < 7u; ++k) {
            out.chunk[i].payload.bytes[k] = (unsigned char)(0x15u + seed * 5u + i * 11u + k * 3u);
        }
    }
    out.guard = (unsigned short)(0x6600u + seed * 23u);
    return out;
}

static Packet step_copy(Packet in, unsigned char salt) {
    Packet out;
    Chunk tmp;

    memcpy(&out, &in, sizeof(out));
    tmp = out.chunk[0];
    out.chunk[0] = out.chunk[3];
    out.chunk[3] = out.chunk[1];
    out.chunk[1] = tmp;
    out.chunk[2].payload.named.d = (unsigned char)(out.chunk[2].payload.named.d + salt);
    out.guard = (unsigned short)(out.guard + out.chunk[2].payload.named.d);
    return out;
}

static Packet step_xor(Packet in, unsigned char salt) {
    Packet out = in;

    out.chunk[0].payload.bytes[6] = (unsigned char)(out.chunk[0].payload.bytes[6] ^ salt);
    out.chunk[1].key = (unsigned short)(out.chunk[1].key + out.chunk[0].payload.named.g);
    out.chunk[3].payload.named.a = (unsigned char)(out.chunk[3].payload.named.a + out.chunk[2].payload.bytes[1]);
    out.guard = (unsigned short)(out.guard ^ (unsigned short)(salt * 37u));
    return out;
}

static PacketStep choose(Packet value, unsigned round) {
    PacketStep steps[2];

    steps[0] = step_copy;
    steps[1] = step_xor;
    return steps[(value.chunk[round & 3u].payload.bytes[(round + 2u) % 7u] + round) & 1u];
}

static unsigned fold_packet(Packet value) {
    unsigned acc = (unsigned)value.guard * 151u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 4u; ++i) {
        acc = acc * 137u + (unsigned)value.chunk[i].key;
        for (k = 0u; k < 7u; ++k) {
            acc = acc * 137u + (unsigned)value.chunk[i].payload.bytes[k];
        }
    }
    return acc;
}

int main(void) {
    Packet packet = seed_packet(8u);
    unsigned i;
    unsigned typed = 0u;

    for (i = 0u; i < 6u; ++i) {
        PacketStep step = choose(packet, i);
        packet = step(packet, (unsigned char)(0x17u + i * 9u));
    }

    typed += (unsigned)packet.guard;
    typed += (unsigned)packet.chunk[0].key * 3u + (unsigned)packet.chunk[0].payload.bytes[6] * 5u;
    typed += (unsigned)packet.chunk[1].key * 7u + (unsigned)packet.chunk[3].payload.named.a * 11u;

    printf("%u %u %u %u %u %u %u %u\n",
           (unsigned)packet.chunk[0].key,
           (unsigned)packet.chunk[0].payload.bytes[6],
           (unsigned)packet.chunk[1].key,
           (unsigned)packet.chunk[2].payload.named.d,
           (unsigned)packet.chunk[3].payload.named.a,
           (unsigned)packet.guard,
           typed,
           fold_packet(packet));
    return 0;
}
