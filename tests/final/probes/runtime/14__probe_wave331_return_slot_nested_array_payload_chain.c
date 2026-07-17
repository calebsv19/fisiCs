#include <stdio.h>

typedef struct Node {
    unsigned char id;
    union {
        struct {
            unsigned char left;
            unsigned char right;
        };
        unsigned short pair;
        unsigned char bytes[2];
    } body;
} Node;

typedef struct Chunk {
    unsigned short stamp;
    Node nodes[4];
    unsigned short tail;
} Chunk;

static Chunk make_chunk(unsigned char seed) {
    Chunk out;
    unsigned i;

    out.stamp = (unsigned short)(0x2500u + seed);
    for (i = 0u; i < 4u; ++i) {
        out.nodes[i].id = (unsigned char)(seed + 0x10u + i * 9u);
        out.nodes[i].body.bytes[0] = (unsigned char)(0x22u + seed + i * 4u);
        out.nodes[i].body.bytes[1] = (unsigned char)(0x55u + seed + i * 6u);
    }
    out.tail = (unsigned short)(0x4700u + seed * 7u);
    return out;
}

static Chunk rewrite_chunk(Chunk in, unsigned char salt) {
    Chunk out = in;
    Node hold = out.nodes[1];

    out.nodes[1] = out.nodes[3];
    out.nodes[3] = hold;
    out.nodes[0].body.left = (unsigned char)(out.nodes[0].body.left + salt);
    out.nodes[2].body.pair = (unsigned short)(out.nodes[2].body.pair ^ (unsigned short)(salt * 19u));
    out.tail = (unsigned short)(out.tail + out.nodes[2].body.bytes[0] + out.nodes[3].body.bytes[1]);
    return out;
}

static Chunk chain(unsigned char seed) {
    Chunk a = make_chunk(seed);
    Chunk b = rewrite_chunk(a, 0x19u);
    Chunk c = rewrite_chunk(b, 0x2Bu);

    c.nodes[0] = rewrite_chunk(c, 0x07u).nodes[2];
    c.stamp = (unsigned short)(c.stamp + c.nodes[0].body.bytes[1]);
    return c;
}

static unsigned fold_chunk(Chunk value) {
    unsigned acc = (unsigned)value.stamp * 157u + (unsigned)value.tail;
    unsigned i;

    for (i = 0u; i < 4u; ++i) {
        acc = acc * 113u + (unsigned)value.nodes[i].id;
        acc = acc * 113u + (unsigned)value.nodes[i].body.bytes[0];
        acc = acc * 113u + (unsigned)value.nodes[i].body.bytes[1];
    }
    return acc;
}

int main(void) {
    Chunk chunk = chain(6u);
    unsigned typed = 0u;

    typed += (unsigned)chunk.stamp + (unsigned)chunk.tail;
    typed += (unsigned)chunk.nodes[0].id * 3u + (unsigned)chunk.nodes[0].body.left * 5u;
    typed += (unsigned)chunk.nodes[1].body.right * 7u + (unsigned)chunk.nodes[2].body.bytes[0] * 11u;
    typed += (unsigned)chunk.nodes[3].id * 13u + (unsigned)chunk.nodes[3].body.bytes[1] * 17u;

    printf("%u %u %u %u %u %u %u %u %u\n",
           (unsigned)chunk.stamp,
           (unsigned)chunk.nodes[0].id,
           (unsigned)chunk.nodes[0].body.bytes[0],
           (unsigned)chunk.nodes[1].body.bytes[1],
           (unsigned)chunk.nodes[2].body.bytes[0],
           (unsigned)chunk.nodes[3].id,
           (unsigned)chunk.tail,
           typed,
           fold_chunk(chunk));
    return 0;
}
