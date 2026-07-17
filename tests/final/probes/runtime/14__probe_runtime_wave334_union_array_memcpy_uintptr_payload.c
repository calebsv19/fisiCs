#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef union Payload {
    unsigned char bytes[sizeof(uintptr_t) + 4u];
    uintptr_t word;
} Payload;

typedef struct Carrier {
    Payload slots[3];
    unsigned short tag;
} Carrier;

static Carrier seed_carrier(uintptr_t base) {
    Carrier out;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 3u; ++i) {
        out.slots[i].word = base + (uintptr_t)(i * 0x111u + 0x25u);
        for (k = sizeof(uintptr_t); k < sizeof(out.slots[i].bytes); ++k) {
            out.slots[i].bytes[k] = (unsigned char)(0x41u + i * 9u + k);
        }
    }
    out.tag = 0x5a31u;
    return out;
}

static Carrier copy_overlay(Carrier in, uintptr_t add) {
    Carrier out;
    Payload tmp;

    memcpy(&out, &in, sizeof(out));
    memcpy(&tmp, &out.slots[0], sizeof(tmp));
    memcpy(&out.slots[0], &out.slots[2], sizeof(out.slots[0]));
    memcpy(&out.slots[2], &tmp, sizeof(out.slots[2]));
    out.slots[1].word = out.slots[1].word + add;
    out.slots[2].bytes[sizeof(uintptr_t)] =
        (unsigned char)(out.slots[2].bytes[sizeof(uintptr_t)] ^ (unsigned char)(add & 0xffu));
    out.tag = (unsigned short)(out.tag + out.slots[0].bytes[sizeof(uintptr_t) + 1u] + out.slots[2].bytes[0]);
    return out;
}

static unsigned fold(Carrier value) {
    unsigned acc = (unsigned)value.tag * 197u;
    unsigned i;
    unsigned k;

    for (i = 0u; i < 3u; ++i) {
        for (k = 0u; k < sizeof(value.slots[i].bytes); ++k) {
            acc = acc * 127u + (unsigned)value.slots[i].bytes[k] + k;
        }
    }
    return acc;
}

int main(void) {
    Carrier carrier = seed_carrier((uintptr_t)0x12340u);
    Carrier copy = copy_overlay(copy_overlay(carrier, (uintptr_t)0x55u), (uintptr_t)0x2bu);
    unsigned typed = 0u;

    typed += (unsigned)(copy.slots[0].word & (uintptr_t)0xffffu) * 3u;
    typed += (unsigned)(copy.slots[1].word & (uintptr_t)0xffffu) * 5u;
    typed += (unsigned)copy.slots[2].bytes[sizeof(uintptr_t)] * 7u;
    typed += (unsigned)copy.tag;

    printf("%u %u %u %u %u %u\n",
           (unsigned)(copy.slots[0].word & (uintptr_t)0xffffu),
           (unsigned)(copy.slots[1].word & (uintptr_t)0xffffu),
           (unsigned)copy.slots[2].bytes[sizeof(uintptr_t)],
           (unsigned)copy.tag,
           typed,
           fold(copy));
    return 0;
}
