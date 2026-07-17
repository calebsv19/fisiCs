#include <stdio.h>

typedef struct Payload {
    unsigned short tag;
    union {
        struct {
            unsigned char a;
            unsigned char b;
            unsigned char c;
            unsigned char d;
        };
        unsigned char raw[4];
    } body;
} Payload;

typedef struct Bundle {
    unsigned short epoch;
    Payload lanes[2][3];
    unsigned short guard;
} Bundle;

static Payload make_payload(unsigned char seed, unsigned short tag_base) {
    Payload out;

    out.tag = (unsigned short)(tag_base + seed * 3u);
    out.body.raw[0] = (unsigned char)(0x11u + seed);
    out.body.raw[1] = (unsigned char)(0x31u + seed * 2u);
    out.body.raw[2] = (unsigned char)(0x51u + seed * 3u);
    out.body.raw[3] = (unsigned char)(0x71u + seed * 5u);
    return out;
}

static Bundle seed_bundle(unsigned char seed) {
    Bundle out;
    unsigned i;
    unsigned j;

    out.epoch = (unsigned short)(0x2200u + seed);
    for (i = 0u; i < 2u; ++i) {
        for (j = 0u; j < 3u; ++j) {
            out.lanes[i][j] = make_payload((unsigned char)(seed + i * 7u + j * 5u),
                                           (unsigned short)(0x4100u + i * 0x80u + j * 0x11u));
        }
    }
    out.guard = (unsigned short)(0x6a00u + seed * 9u);
    return out;
}

static Bundle rewrite_bundle(Bundle in, unsigned char salt) {
    Bundle out = in;
    Payload hold = out.lanes[0][2];

    out.lanes[0][2] = out.lanes[1][0];
    out.lanes[1][0] = hold;
    out.lanes[0][1].body.b = (unsigned char)(out.lanes[0][1].body.b + salt);
    out.lanes[1][2].body.raw[3] = (unsigned char)(out.lanes[1][2].body.raw[3] ^ (salt + out.lanes[0][0].body.a));
    out.guard = (unsigned short)(out.guard + out.lanes[1][0].body.c + out.lanes[0][2].tag);
    return out;
}

static Bundle build_chain(unsigned char seed) {
    Bundle first = seed_bundle(seed);
    Bundle second = rewrite_bundle(first, 0x17u);
    Bundle third = rewrite_bundle(second, 0x2du);

    third.lanes[0][0] = rewrite_bundle(third, 0x09u).lanes[1][1];
    third.epoch = (unsigned short)(third.epoch + third.lanes[0][0].body.raw[2]);
    return third;
}

static unsigned fold_bundle(Bundle value) {
    unsigned acc = (unsigned)value.epoch * 173u + (unsigned)value.guard;
    unsigned i;
    unsigned j;
    unsigned k;

    for (i = 0u; i < 2u; ++i) {
        for (j = 0u; j < 3u; ++j) {
            acc = acc * 127u + (unsigned)value.lanes[i][j].tag;
            for (k = 0u; k < 4u; ++k) {
                acc = acc * 89u + (unsigned)value.lanes[i][j].body.raw[k] + k;
            }
        }
    }
    return acc;
}

int main(void) {
    Bundle bundle = build_chain(5u);
    unsigned typed = 0u;

    typed += (unsigned)bundle.epoch + (unsigned)bundle.guard;
    typed += (unsigned)bundle.lanes[0][0].tag * 3u + (unsigned)bundle.lanes[0][1].body.b * 5u;
    typed += (unsigned)bundle.lanes[1][0].body.raw[2] * 7u + (unsigned)bundle.lanes[1][2].body.d * 11u;

    printf("%u %u %u %u %u %u %u %u %u\n",
           (unsigned)bundle.epoch,
           (unsigned)bundle.lanes[0][0].tag,
           (unsigned)bundle.lanes[0][0].body.raw[2],
           (unsigned)bundle.lanes[0][1].body.raw[1],
           (unsigned)bundle.lanes[1][0].tag,
           (unsigned)bundle.lanes[1][0].body.raw[2],
           (unsigned)bundle.guard,
           typed,
           fold_bundle(bundle));
    return 0;
}
