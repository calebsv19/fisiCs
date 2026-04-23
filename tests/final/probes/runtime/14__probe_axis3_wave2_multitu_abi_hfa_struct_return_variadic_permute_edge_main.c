#include <stdio.h>

struct Axis3W2Hfa {
    float x;
    float y;
    float z;
    float w;
};

struct Axis3W2Hfa axis3_w2_hfa_collect(float seed, int count, ...);
unsigned axis3_w2_hfa_digest(struct Axis3W2Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W2Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 10; ++i) {
        last = axis3_w2_hfa_collect(
            0.5f + (float)i * 0.1875f,
            8,
            0, 0.75 + (double)i * 0.25,
            1, 5 + i * 2,
            0, 1.125 + (double)i * 0.125,
            1, 11 - i,
            0, 2.25 + (double)i * 0.0625,
            1, 17 + i,
            0, 3.5 + (double)i * 0.03125,
            1, 23 + i * 3
        );

        acc ^= axis3_w2_hfa_digest(last, (unsigned)(i * 29 + 7));
        acc = rotl32(acc + (unsigned)(i * 19 + 13), 9u);
    }

    printf("%u %u\n", acc, axis3_w2_hfa_digest(last, 131u));
    return 0;
}
