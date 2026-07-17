#include <stdio.h>

typedef struct Lane {
    unsigned char tag;
    union {
        struct {
            unsigned char a;
            unsigned char b;
            unsigned char c;
            unsigned char d;
        };
        unsigned int word;
        unsigned char raw[4];
    } payload;
} Lane;

typedef struct Bundle {
    unsigned short head;
    Lane lanes[3];
    unsigned short tail;
} Bundle;

typedef Bundle (*BundleStep)(Bundle, unsigned char);

static unsigned hash_bundle(Bundle value) {
    unsigned acc = (unsigned)value.head * 257u + (unsigned)value.tail;
    unsigned i;
    unsigned j;

    for (i = 0u; i < 3u; ++i) {
        acc = acc * 193u + (unsigned)value.lanes[i].tag;
        for (j = 0u; j < 4u; ++j) {
            acc = acc * 97u + (unsigned)value.lanes[i].payload.raw[j] + (i * 13u + j);
        }
    }
    return acc;
}

static Bundle make_bundle(unsigned char seed) {
    Bundle out;
    unsigned i;
    unsigned j;

    out.head = (unsigned short)(0x1200u + seed);
    for (i = 0u; i < 3u; ++i) {
        out.lanes[i].tag = (unsigned char)(seed + 0x30u + i * 4u);
        for (j = 0u; j < 4u; ++j) {
            out.lanes[i].payload.raw[j] = (unsigned char)(0x40u + seed + i * 11u + j * 7u);
        }
    }
    out.tail = (unsigned short)(0x3400u + seed);
    return out;
}

static Bundle mix_lane_bytes(Bundle in, unsigned char salt) {
    Bundle out = in;

    out.lanes[0].payload.raw[3] ^= salt;
    out.lanes[1].payload.word += (unsigned int)(salt * 0x01010101u);
    out.lanes[2].payload.raw[0] = (unsigned char)(out.lanes[2].payload.raw[0] + out.lanes[0].payload.raw[1]);
    out.tail = (unsigned short)(out.tail + out.lanes[1].payload.raw[2] + salt);
    return out;
}

static Bundle swap_and_retag(Bundle in, unsigned char salt) {
    Bundle out = in;
    Lane saved = out.lanes[0];

    out.lanes[0] = out.lanes[2];
    out.lanes[2] = saved;
    out.lanes[1].tag = (unsigned char)(out.lanes[1].tag ^ salt);
    out.head = (unsigned short)(out.head + out.lanes[0].payload.raw[0]);
    return out;
}

static Bundle run_bundle(Bundle base, BundleStep first, BundleStep second) {
    Bundle current = first(base, 0x19u);
    current = second(current, 0x2Bu);
    current = first(current, 0x35u);
    return current;
}

int main(void) {
    Bundle value = run_bundle(make_bundle(9u), mix_lane_bytes, swap_and_retag);
    unsigned typed = 0u;

    typed += (unsigned)value.head;
    typed += (unsigned)value.lanes[0].payload.a * 3u + (unsigned)value.lanes[0].payload.d * 5u;
    typed += (unsigned)value.lanes[1].tag * 7u + (unsigned)value.lanes[1].payload.b * 11u;
    typed += (unsigned)value.lanes[2].payload.raw[0] * 13u + (unsigned)value.lanes[2].payload.raw[3] * 17u;
    typed += (unsigned)value.tail;

    printf("%u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)value.head,
           (unsigned)value.lanes[0].tag,
           (unsigned)value.lanes[0].payload.raw[0],
           (unsigned)value.lanes[0].payload.raw[3],
           (unsigned)value.lanes[1].tag,
           (unsigned)value.lanes[1].payload.raw[1],
           (unsigned)value.lanes[2].tag,
           (unsigned)value.lanes[2].payload.raw[0],
           typed,
           hash_bundle(value));
    return 0;
}
