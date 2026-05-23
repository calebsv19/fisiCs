#include <stdio.h>

struct Axis3W26Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W26Hfa axis3_w26_hfa_collect(
    float seed,
    unsigned shadow,
    unsigned replay,
    unsigned checkpoint,
    int count,
    ...
);
unsigned axis3_w26_hfa_digest(struct Axis3W26Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x57f19c43u;
    struct Axis3W26Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 32; ++i) {
        last = axis3_w26_hfa_collect(
            1.5625f + (float)i * 0.0546875f,
            (unsigned)(53 + i * 29),
            (unsigned)(37 + i * 23),
            (unsigned)(149 + i * 17),
            23,
            2, 257u + (unsigned)i * 31u,
            0, 1.015625 + (double)i * 0.08203125,
            1, 89 + i * 7,
            0, 1.875 + (double)i * 0.041015625,
            2, 311u + (unsigned)i * 29u,
            1, 113 - i,
            0, 2.453125 + (double)i * 0.0205078125,
            2, 359u + (unsigned)i * 23u,
            1, 139 + i * 2,
            0, 2.9375 + (double)i * 0.01025390625,
            2, 409u + (unsigned)i * 19u,
            1, 167 + i,
            0, 3.328125 + (double)i * 0.005126953125,
            2, 457u + (unsigned)i * 17u,
            1, 193 + i * 3,
            0, 3.640625 + (double)i * 0.0025634765625,
            2, 509u + (unsigned)i * 13u,
            1, 221 - i,
            0, 3.90625 + (double)i * 0.00128173828125,
            2, 563u + (unsigned)i * 11u,
            1, 247 + i * 2,
            0, 4.125 + (double)i * 0.000640869140625,
            2, 607u + (unsigned)i * 7u,
            1, 271 + i
        );

        acc ^= axis3_w26_hfa_digest(last, (unsigned)(i * 139 + 97));
        acc = rotl32(acc + (unsigned)(i * 103 + 27), 13u);
    }

    printf("%u %u\n", acc, axis3_w26_hfa_digest(last, 1493u));
    return 0;
}
