#include <stdio.h>

struct Axis3W19ReducedHfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W19ReducedHfa axis3_w19_reduced_collect(
    float seed,
    unsigned epoch,
    unsigned shadow,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w19_reduced_digest(struct Axis3W19ReducedHfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x51f15e3bu;
    struct Axis3W19ReducedHfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 8; ++i) {
        last = axis3_w19_reduced_collect(
            1.25f + (float)i * 0.0625f,
            (unsigned)(111 + i * 23),
            (unsigned)(17 + i * 19),
            (unsigned)(7 + i * 13),
            6,
            0, 1.5 + (double)i * 0.25,
            1, 33 + i * 5,
            2, 91u + (unsigned)i * 17u,
            0, 2.75 + (double)i * 0.125,
            1, 47 + i * 3,
            2, 131u + (unsigned)i * 7u
        );

        acc ^= axis3_w19_reduced_digest(last, (unsigned)(i * 43 + 19));
        acc = rotl32(acc + (unsigned)(i * 29 + 17), 5u);
    }

    printf("%u %u\n", acc, axis3_w19_reduced_digest(last, 313u));
    return 0;
}
