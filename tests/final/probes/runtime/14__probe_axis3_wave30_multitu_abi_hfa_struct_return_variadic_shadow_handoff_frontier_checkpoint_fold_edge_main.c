#include <stdio.h>

struct Axis3W30Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W30Hfa axis3_w30_hfa_collect(
    float seed,
    unsigned shadow,
    unsigned handoff,
    unsigned frontier,
    unsigned checkpoint,
    int count,
    ...
);
unsigned axis3_w30_hfa_digest(struct Axis3W30Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x6a31d48fu;
    struct Axis3W30Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 34; ++i) {
        last = axis3_w30_hfa_collect(
            1.5390625f + (float)i * 0.0517578125f,
            (unsigned)(79 + i * 23),
            (unsigned)(17 + i * 31),
            (unsigned)(53 + i * 19),
            (unsigned)(137 + i * 13),
            23,
            2, 239u + (unsigned)i * 29u,
            0, 1.078125 + (double)i * 0.0732421875,
            1, 103 + i * 4,
            0, 1.734375 + (double)i * 0.03662109375,
            2, 287u + (unsigned)i * 23u,
            1, 129 - i,
            0, 2.296875 + (double)i * 0.018310546875,
            2, 349u + (unsigned)i * 19u,
            1, 159 + i * 2,
            0, 2.796875 + (double)i * 0.0091552734375,
            2, 401u + (unsigned)i * 17u,
            1, 181 + i,
            0, 3.1875 + (double)i * 0.00457763671875,
            2, 443u + (unsigned)i * 13u,
            1, 213 + i * 3,
            0, 3.53125 + (double)i * 0.002288818359375,
            2, 491u + (unsigned)i * 11u,
            1, 241 - i,
            0, 3.8125 + (double)i * 0.0011444091796875,
            2, 547u + (unsigned)i * 7u,
            1, 267 + i * 2,
            0, 4.046875 + (double)i * 0.00057220458984375,
            2, 593u + (unsigned)i * 5u,
            1, 281 + i
        );

        acc ^= axis3_w30_hfa_digest(last, (unsigned)(i * 167 + 71));
        acc = rotl32(acc + (unsigned)(i * 109 + 37), 9u);
    }

    printf("%u %u\n", acc, axis3_w30_hfa_digest(last, 1901u));
    return 0;
}
