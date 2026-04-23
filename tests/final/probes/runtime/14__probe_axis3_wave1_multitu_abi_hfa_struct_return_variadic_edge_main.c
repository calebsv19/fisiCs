#include <stdio.h>

struct Axis3W1Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W1Hfa axis3_w1_hfa_collect(float seed, int lanes, ...);
unsigned axis3_w1_hfa_digest(struct Axis3W1Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W1Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 9; ++i) {
        last = axis3_w1_hfa_collect(
            0.75f + (float)i * 0.125f,
            6,
            0, 1.25 + (double)i * 0.25,
            1, 3 + i * 2,
            0, 0.5 + (double)i * 0.125,
            1, 7 - i,
            0, 2.0 + (double)i * 0.0625,
            1, 11 + i
        );

        acc ^= axis3_w1_hfa_digest(last, (unsigned)(i * 17 + 5));
        acc = rotl32(acc + (unsigned)(i * 31 + 9), 7u);
    }

    printf("%u %u\n", acc, axis3_w1_hfa_digest(last, 97u));
    return 0;
}
