#include <stdio.h>

struct Axis3W33Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W33Hfa axis3_w33_hfa_collect(
    float seed,
    unsigned shadow,
    unsigned checkpoint,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w33_hfa_digest(struct Axis3W33Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x5ac3d917u;
    struct Axis3W33Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 36; ++i) {
        last = axis3_w33_hfa_collect(
            1.484375f + (float)i * 0.04931640625f,
            (unsigned)(89 + i * 17),
            (unsigned)(143 + i * 11),
            (unsigned)(47 + i * 19),
            (unsigned)(29 + i * 31),
            23,
            2, 233u + (unsigned)i * 29u,
            0, 1.09375 + (double)i * 0.07470703125,
            1, 97 + i * 4,
            0, 1.765625 + (double)i * 0.037353515625,
            2, 287u + (unsigned)i * 23u,
            1, 123 - i,
            0, 2.34375 + (double)i * 0.0186767578125,
            2, 349u + (unsigned)i * 19u,
            1, 151 + i * 2,
            0, 2.828125 + (double)i * 0.00933837890625,
            2, 401u + (unsigned)i * 17u,
            1, 183 + i,
            0, 3.21875 + (double)i * 0.004669189453125,
            2, 449u + (unsigned)i * 13u,
            1, 211 + i * 3,
            0, 3.546875 + (double)i * 0.0023345947265625,
            2, 503u + (unsigned)i * 11u,
            1, 237 - i,
            0, 3.8125 + (double)i * 0.00116729736328125,
            2, 557u + (unsigned)i * 7u,
            1, 261 + i * 2,
            0, 4.046875 + (double)i * 0.000583648681640625,
            2, 601u + (unsigned)i * 5u,
            1, 283 + i
        );

        acc ^= axis3_w33_hfa_digest(last, (unsigned)(i * 181 + 59));
        acc = rotl32(acc + (unsigned)(i * 113 + 39), 7u);
    }

    printf("%u %u\n", acc, axis3_w33_hfa_digest(last, 2143u));
    return 0;
}
