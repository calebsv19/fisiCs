#include <stdio.h>

struct Axis3W28Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W28Hfa axis3_w28_hfa_collect(
    float seed,
    unsigned frontier,
    unsigned checkpoint,
    unsigned shadow,
    unsigned handoff,
    int count,
    ...
);
unsigned axis3_w28_hfa_digest(struct Axis3W28Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x61d4a2b7u;
    struct Axis3W28Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 33; ++i) {
        last = axis3_w28_hfa_collect(
            1.5078125f + (float)i * 0.052734375f,
            (unsigned)(47 + i * 17),
            (unsigned)(131 + i * 11),
            (unsigned)(73 + i * 29),
            (unsigned)(19 + i * 23),
            23,
            2, 233u + (unsigned)i * 31u,
            0, 1.0625 + (double)i * 0.07421875,
            1, 101 + i * 4,
            0, 1.703125 + (double)i * 0.037109375,
            2, 281u + (unsigned)i * 27u,
            1, 127 - i,
            0, 2.28125 + (double)i * 0.0185546875,
            2, 347u + (unsigned)i * 21u,
            1, 157 + i * 2,
            0, 2.78125 + (double)i * 0.00927734375,
            2, 397u + (unsigned)i * 17u,
            1, 179 + i,
            0, 3.171875 + (double)i * 0.004638671875,
            2, 443u + (unsigned)i * 13u,
            1, 211 + i * 3,
            0, 3.515625 + (double)i * 0.0023193359375,
            2, 491u + (unsigned)i * 11u,
            1, 239 - i,
            0, 3.796875 + (double)i * 0.00115966796875,
            2, 547u + (unsigned)i * 7u,
            1, 263 + i * 2,
            0, 4.03125 + (double)i * 0.000579833984375,
            2, 593u + (unsigned)i * 5u,
            1, 277 + i
        );

        acc ^= axis3_w28_hfa_digest(last, (unsigned)(i * 163 + 83));
        acc = rotl32(acc + (unsigned)(i * 101 + 41), 11u);
    }

    printf("%u %u\n", acc, axis3_w28_hfa_digest(last, 1729u));
    return 0;
}
