#include <stdio.h>

typedef struct Segment {
    unsigned char lane;
    union {
        struct {
            unsigned char a;
            unsigned char b;
            unsigned char c;
            unsigned char d;
        };
        unsigned int word;
        unsigned char raw[4];
    } bytes;
} Segment;

typedef struct Record {
    unsigned short head;
    Segment slots[4];
    unsigned short guard;
} Record;

static Segment make_segment(unsigned char base, unsigned char salt) {
    Segment out;

    out.lane = (unsigned char)(0x10u + base);
    out.bytes.raw[0] = (unsigned char)(base + salt);
    out.bytes.raw[1] = (unsigned char)(base ^ salt);
    out.bytes.raw[2] = (unsigned char)(0xB0u + base - salt);
    out.bytes.raw[3] = (unsigned char)(out.bytes.a + out.bytes.b + out.bytes.c);
    return out;
}

static Record make_record(unsigned char seed) {
    Record out;
    unsigned i;

    out.head = (unsigned short)(0x1500u + seed);
    out.guard = (unsigned short)(0x5100u + seed * 5u);
    for (i = 0u; i < 4u; ++i) {
        out.slots[i] = make_segment((unsigned char)(seed + i * 8u), (unsigned char)(0x21u + i * 3u));
    }
    return out;
}

static Record rewrite_record(Record in) {
    Record out = in;
    Segment returned = make_segment(0x37u, 0x19u);

    out.slots[2] = out.slots[0];
    out.slots[0] = returned;
    out.slots[1].bytes.word += 0x01020304u;
    out.slots[3].bytes.raw[2] ^= out.slots[0].bytes.raw[1];
    out.guard = (unsigned short)(out.guard + out.slots[1].bytes.c + out.slots[3].bytes.d);
    return out;
}

static unsigned fold_record(Record value) {
    unsigned acc = (unsigned)value.head * 229u + (unsigned)value.guard;
    unsigned i;
    unsigned j;

    for (i = 0u; i < 4u; ++i) {
        acc = acc * 157u + (unsigned)value.slots[i].lane;
        for (j = 0u; j < 4u; ++j) {
            acc = acc * 89u + (unsigned)value.slots[i].bytes.raw[j] + i + j;
        }
    }
    return acc;
}

int main(void) {
    Record value = rewrite_record(make_record(3u));
    unsigned typed = 0u;

    typed += (unsigned)value.head + (unsigned)value.guard;
    typed += (unsigned)value.slots[0].bytes.a * 3u + (unsigned)value.slots[0].bytes.d * 5u;
    typed += (unsigned)value.slots[1].bytes.b * 7u + (unsigned)value.slots[2].bytes.c * 11u;
    typed += (unsigned)value.slots[3].lane * 13u + (unsigned)value.slots[3].bytes.raw[2] * 17u;

    printf("%u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)value.head,
           (unsigned)value.slots[0].lane,
           (unsigned)value.slots[0].bytes.raw[0],
           (unsigned)value.slots[0].bytes.raw[3],
           (unsigned)value.slots[1].bytes.raw[1],
           (unsigned)value.slots[2].lane,
           (unsigned)value.slots[3].bytes.raw[2],
           (unsigned)value.guard,
           typed,
           fold_record(value));
    return 0;
}
