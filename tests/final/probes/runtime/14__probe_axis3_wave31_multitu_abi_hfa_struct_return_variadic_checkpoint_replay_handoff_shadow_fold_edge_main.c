#include <stdio.h>

struct Axis3W31Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W31Hfa axis3_w31_hfa_collect(
    float seed,
    unsigned checkpoint,
    unsigned replay,
    unsigned handoff,
    unsigned shadow,
    int count,
    ...
);
unsigned axis3_w31_hfa_digest(struct Axis3W31Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x5f28b1d9u;
    struct Axis3W31Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 36; ++i) {
        last = axis3_w31_hfa_collect(
            1.4921875f + (float)i * 0.0498046875f,
            (unsigned)(139 + i * 13),
            (unsigned)(31 + i * 29),
            (unsigned)(19 + i * 23),
            (unsigned)(83 + i * 17),
            23,
            2, 229u + (unsigned)i * 31u,
            0, 1.1015625 + (double)i * 0.0751953125,
            1, 95 + i * 5,
            0, 1.78125 + (double)i * 0.03759765625,
            2, 291u + (unsigned)i * 23u,
            1, 121 - i,
            0, 2.359375 + (double)i * 0.018798828125,
            2, 347u + (unsigned)i * 19u,
            1, 149 + i * 2,
            0, 2.84375 + (double)i * 0.0093994140625,
            2, 401u + (unsigned)i * 17u,
            1, 181 + i,
            0, 3.234375 + (double)i * 0.00469970703125,
            2, 449u + (unsigned)i * 13u,
            1, 209 + i * 3,
            0, 3.5625 + (double)i * 0.002349853515625,
            2, 503u + (unsigned)i * 11u,
            1, 237 - i,
            0, 3.828125 + (double)i * 0.0011749267578125,
            2, 557u + (unsigned)i * 7u,
            1, 261 + i * 2,
            0, 4.0625 + (double)i * 0.00058746337890625,
            2, 601u + (unsigned)i * 5u,
            1, 283 + i
        );

        acc ^= axis3_w31_hfa_digest(last, (unsigned)(i * 173 + 67));
        acc = rotl32(acc + (unsigned)(i * 107 + 43), 13u);
    }

    printf("%u %u\n", acc, axis3_w31_hfa_digest(last, 1999u));
    return 0;
}
