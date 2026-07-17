#include <stdio.h>

typedef union Payload {
    unsigned char b[8];
    unsigned short h[4];
} Payload;

typedef struct Node {
    unsigned short id;
    Payload payload;
} Node;

typedef struct Envelope {
    Node node[3];
    unsigned short guard;
} Envelope;

static Node make_node(unsigned seed) {
    Node out;
    unsigned i;

    out.id = (unsigned short)(0x3200u + seed * 17u);
    for (i = 0u; i < 8u; ++i) {
        out.payload.b[i] = (unsigned char)(0x21u + seed * 9u + i * 5u);
    }
    return out;
}

static Envelope make_envelope(unsigned seed) {
    Envelope out;
    unsigned i;

    for (i = 0u; i < 3u; ++i) {
        out.node[i] = make_node(seed + i * 4u);
    }
    out.guard = (unsigned short)(0x7100u + seed * 13u);
    return out;
}

static Envelope rotate_return_slot(Envelope in, unsigned salt) {
    Envelope out = in;
    Node keep = out.node[2];

    out.node[2] = out.node[0];
    out.node[0] = out.node[1];
    out.node[1] = keep;
    out.node[0].payload.h[2] = (unsigned short)(out.node[0].payload.h[2] + salt);
    out.node[1].payload.b[3] = (unsigned char)(out.node[1].payload.b[3] ^ (unsigned char)(salt >> 1));
    out.guard = (unsigned short)(out.guard + out.node[0].payload.b[4] + out.node[2].payload.b[6]);
    return out;
}

static unsigned fold_envelope(Envelope value) {
    unsigned acc = (unsigned)value.guard * 181u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 3u; ++i) {
        acc = acc * 113u + (unsigned)value.node[i].id;
        for (k = 0u; k < 8u; ++k) {
            acc = acc * 113u + (unsigned)value.node[i].payload.b[k] + k;
        }
    }
    return acc;
}

int main(void) {
    Envelope env = make_envelope(6u);
    Envelope copy = rotate_return_slot(rotate_return_slot(env, 0x35u), 0x19u);
    unsigned typed = 0u;

    typed += (unsigned)copy.node[0].id * 3u;
    typed += (unsigned)copy.node[0].payload.b[4] * 5u;
    typed += (unsigned)copy.node[1].payload.h[1] * 7u;
    typed += (unsigned)copy.node[2].payload.b[6] * 11u;
    typed += (unsigned)copy.guard;

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)copy.node[0].id,
           (unsigned)copy.node[0].payload.b[4],
           (unsigned)copy.node[1].payload.h[1],
           (unsigned)copy.node[2].payload.b[6],
           (unsigned)copy.guard,
           typed,
           fold_envelope(copy));
    return 0;
}
