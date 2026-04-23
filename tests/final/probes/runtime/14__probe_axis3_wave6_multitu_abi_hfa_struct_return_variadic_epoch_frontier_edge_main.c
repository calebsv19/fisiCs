#include <stdio.h>

struct Axis3W6Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W6Hfa axis3_w6_hfa_collect(float seed, unsigned epoch, unsigned frontier, int count, ...);
unsigned axis3_w6_hfa_digest(struct Axis3W6Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W6Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 10; ++i) {
        last = axis3_w6_hfa_collect(
            0.875f + (float)i * 0.15625f,
            (unsigned)(41 + i * 13),
            (unsigned)(17 + i * 7),
            12,
            0, 0.5 + (double)i * 0.25,
            1, 9 + i * 2,
            2, 47u + (unsigned)i * 11u,
            0, 1.375 + (double)i * 0.125,
            1, 21 - i,
            2, 61u + (unsigned)i * 9u,
            0, 2.25 + (double)i * 0.0625,
            1, 33 + i,
            2, 79u + (unsigned)i * 5u,
            0, 3.125 + (double)i * 0.03125,
            1, 43 + i * 3,
            2, 97u + (unsigned)i * 3u
        );

        acc ^= axis3_w6_hfa_digest(last, (unsigned)(i * 37 + 11));
        acc = rotl32(acc + (unsigned)(i * 29 + 5), 13u);
    }

    printf("%u %u\n", acc, axis3_w6_hfa_digest(last, 263u));
    return 0;
}
