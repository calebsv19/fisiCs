#include <stdio.h>

struct Axis3W29Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W29Hfa axis3_w29_hfa_collect(
    float seed,
    unsigned replay,
    unsigned frontier,
    unsigned checkpoint,
    unsigned shadow,
    int count,
    ...
);
unsigned axis3_w29_hfa_digest(struct Axis3W29Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x58c9f143u;
    struct Axis3W29Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 35; ++i) {
        last = axis3_w29_hfa_collect(
            1.46875f + (float)i * 0.048828125f,
            (unsigned)(23 + i * 31),
            (unsigned)(59 + i * 19),
            (unsigned)(149 + i * 13),
            (unsigned)(71 + i * 17),
            23,
            2, 227u + (unsigned)i * 29u,
            0, 1.09375 + (double)i * 0.076171875,
            1, 91 + i * 5,
            0, 1.765625 + (double)i * 0.0380859375,
            2, 293u + (unsigned)i * 23u,
            1, 123 - i,
            0, 2.34375 + (double)i * 0.01904296875,
            2, 349u + (unsigned)i * 19u,
            1, 151 + i * 2,
            0, 2.828125 + (double)i * 0.009521484375,
            2, 397u + (unsigned)i * 17u,
            1, 183 + i,
            0, 3.21875 + (double)i * 0.0047607421875,
            2, 449u + (unsigned)i * 13u,
            1, 207 + i * 3,
            0, 3.546875 + (double)i * 0.00238037109375,
            2, 503u + (unsigned)i * 11u,
            1, 233 - i,
            0, 3.8125 + (double)i * 0.001190185546875,
            2, 557u + (unsigned)i * 7u,
            1, 257 + i * 2,
            0, 4.046875 + (double)i * 0.0005950927734375,
            2, 601u + (unsigned)i * 5u,
            1, 279 + i
        );

        acc ^= axis3_w29_hfa_digest(last, (unsigned)(i * 157 + 101));
        acc = rotl32(acc + (unsigned)(i * 89 + 55), 7u);
    }

    printf("%u %u\n", acc, axis3_w29_hfa_digest(last, 1801u));
    return 0;
}
