#include <stdio.h>

typedef struct Payload {
    unsigned short tag;
    struct {
        union {
            struct {
                unsigned char a;
                unsigned char b;
                unsigned char c;
                unsigned char d;
            };
            unsigned int word;
            unsigned char bytes[4];
        };
    } nested;
    unsigned short tail;
} Payload;

static unsigned byte_sum_payload(Payload value) {
    const unsigned char *p = value.nested.bytes;
    unsigned sum = 0u;
    unsigned i;

    sum = (unsigned)value.tag + ((unsigned)value.tail << 3);
    for (i = 0u; i < sizeof(value.nested.bytes); ++i) {
        sum = (sum * 131u) + (unsigned)p[i] + (unsigned)(i + 17u);
    }
    return sum;
}

static Payload copy_and_retag(Payload in, unsigned char flip) {
    Payload out = in;
    out.nested.bytes[1] ^= flip;
    out.tail = (unsigned short)(out.tail + out.nested.a + out.nested.d);
    return out;
}

int main(void) {
    Payload first;
    Payload second;
    unsigned typed;
    unsigned bytes;

    first.tag = 0x1234u;
    first.nested.a = 0xA1u;
    first.nested.b = 0xB2u;
    first.nested.c = 0xC3u;
    first.nested.d = 0xD4u;
    first.tail = 0x4567u;

    second = first;
    second = copy_and_retag(second, 0x5Au);

    typed = ((unsigned)second.tag << 1)
        ^ ((unsigned)second.nested.a << 3)
        ^ ((unsigned)second.nested.b << 5)
        ^ ((unsigned)second.nested.c << 7)
        ^ ((unsigned)second.nested.d << 9)
        ^ (unsigned)second.tail;
    bytes = byte_sum_payload(second);

    printf("%u %u %u %u %u %u %u\n",
           (unsigned)second.tag,
           (unsigned)second.nested.a,
           (unsigned)second.nested.b,
           (unsigned)second.nested.c,
           (unsigned)second.nested.d,
           typed,
           bytes);
    return 0;
}
