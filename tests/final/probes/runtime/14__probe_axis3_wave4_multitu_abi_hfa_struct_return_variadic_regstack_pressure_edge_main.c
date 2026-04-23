#include <stdio.h>

struct Axis3W4Hfa {
    float x;
    float y;
    float z;
    float w;
};

struct Axis3W4Hfa axis3_w4_hfa_collect(float seed, unsigned phase, int count, ...);
unsigned axis3_w4_hfa_digest(struct Axis3W4Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W4Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 10; ++i) {
        last = axis3_w4_hfa_collect(
            0.5f + (float)i * 0.125f,
            (unsigned)(23 + i * 7),
            12,
            0, 0.625 + (double)i * 0.25,
            1, 9 + i,
            2, 31u + (unsigned)i * 11u,
            0, 1.25 + (double)i * 0.125,
            1, 17 - i,
            2, 47u + (unsigned)i * 9u,
            0, 2.125 + (double)i * 0.0625,
            1, 29 + i * 2,
            2, 59u + (unsigned)i * 5u,
            0, 3.0 + (double)i * 0.03125,
            1, 41 + i * 3,
            2, 71u + (unsigned)i * 3u
        );

        acc ^= axis3_w4_hfa_digest(last, (unsigned)(i * 37 + 7));
        acc = rotl32(acc + (unsigned)(i * 29 + 5), 11u);
    }

    printf("%u %u\n", acc, axis3_w4_hfa_digest(last, 211u));
    return 0;
}
