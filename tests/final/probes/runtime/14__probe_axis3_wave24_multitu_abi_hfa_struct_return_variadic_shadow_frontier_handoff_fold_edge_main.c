#include <stdio.h>

struct Axis3W24Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W24Hfa axis3_w24_hfa_collect(
    float seed,
    unsigned shadow,
    unsigned frontier,
    unsigned handoff,
    int count,
    ...
);
unsigned axis3_w24_hfa_digest(struct Axis3W24Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x5d4c921fu;
    struct Axis3W24Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 30; ++i) {
        last = axis3_w24_hfa_collect(
            1.453125f + (float)i * 0.05859375f,
            (unsigned)(47 + i * 31),
            (unsigned)(193 + i * 17),
            (unsigned)(71 + i * 11),
            25,
            2, 227u + (unsigned)i * 29u,
            0, 1.078125 + (double)i * 0.08984375,
            1, 77 + i * 5,
            0, 1.984375 + (double)i * 0.044921875,
            2, 271u + (unsigned)i * 23u,
            1, 101 - i,
            0, 2.625 + (double)i * 0.0224609375,
            2, 317u + (unsigned)i * 19u,
            1, 127 + i * 2,
            0, 3.15625 + (double)i * 0.01123046875,
            2, 359u + (unsigned)i * 17u,
            1, 151 + i,
            0, 3.578125 + (double)i * 0.005615234375,
            2, 409u + (unsigned)i * 13u,
            1, 179 + i * 3,
            0, 3.921875 + (double)i * 0.0028076171875,
            2, 457u + (unsigned)i * 11u,
            1, 203 - i,
            0, 4.203125 + (double)i * 0.00140380859375,
            2, 503u + (unsigned)i * 7u,
            1, 229 + i * 2,
            0, 4.421875 + (double)i * 0.000701904296875,
            2, 547u + (unsigned)i * 5u,
            1, 257 + i
        );

        acc ^= axis3_w24_hfa_digest(last, (unsigned)(i * 131 + 79));
        acc = rotl32(acc + (unsigned)(i * 97 + 23), 9u);
    }

    printf("%u %u\n", acc, axis3_w24_hfa_digest(last, 1367u));
    return 0;
}
