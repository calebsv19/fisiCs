#include <stdio.h>

struct Axis3W18Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W18Hfa axis3_w18_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned shadow,
    int count,
    ...
);
unsigned axis3_w18_hfa_digest(struct Axis3W18Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x7f4a7c15u;
    struct Axis3W18Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 23; ++i) {
        last = axis3_w18_hfa_collect(
            1.09375f + (float)i * 0.0791015625f,
            (unsigned)(241 + i * 43),
            (unsigned)(27 + i * 29),
            (unsigned)(13 + i * 31),
            25,
            2, 149u + (unsigned)i * 41u,
            0, 0.9375 + (double)i * 0.1328125,
            1, 39 + i * 19,
            0, 2.28125 + (double)i * 0.06640625,
            2, 233u + (unsigned)i * 23u,
            1, 71 - i,
            0, 3.1875 + (double)i * 0.03125,
            2, 281u + (unsigned)i * 17u,
            1, 97 + i,
            0, 3.78125 + (double)i * 0.015625,
            1, 113 + i * 2,
            2, 331u + (unsigned)i * 13u,
            0, 4.34375 + (double)i * 0.0078125,
            1, 139 + i,
            2, 373u + (unsigned)i * 11u,
            0, 4.78125 + (double)i * 0.00390625,
            1, 163 + i * 3,
            2, 419u + (unsigned)i * 7u,
            0, 5.125 + (double)i * 0.001953125,
            1, 191 - i,
            2, 463u + (unsigned)i * 5u,
            0, 5.40625 + (double)i * 0.0009765625,
            1, 211 + i * 2
        );

        acc ^= axis3_w18_hfa_digest(last, (unsigned)(i * 97 + 73));
        acc = rotl32(acc + (unsigned)(i * 83 + 47), 9u);
    }

    printf("%u %u\n", acc, axis3_w18_hfa_digest(last, 941u));
    return 0;
}
