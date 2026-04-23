#include <stdio.h>

struct Axis3W5Hfa {
    float p;
    float q;
    float r;
    float s;
};

struct Axis3W5Hfa axis3_w5_hfa_collect(float seed, unsigned checkpoint, unsigned rotate, int count, ...);
unsigned axis3_w5_hfa_digest(struct Axis3W5Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W5Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 9; ++i) {
        last = axis3_w5_hfa_collect(
            0.75f + (float)i * 0.1875f,
            (unsigned)(37 + i * 13),
            (unsigned)(i & 7),
            11,
            0, 0.875 + (double)i * 0.125,
            1, 7 + i * 3,
            2, 41u + (unsigned)i * 5u,
            0, 1.5 + (double)i * 0.0625,
            1, 13 + i * 2,
            2, 53u + (unsigned)i * 7u,
            0, 2.75 + (double)i * 0.03125,
            1, 29 + i,
            2, 67u + (unsigned)i * 3u,
            1, 43 + i * 2,
            2, 79u + (unsigned)i
        );

        acc ^= axis3_w5_hfa_digest(last, (unsigned)(i * 43 + 11));
        acc = rotl32(acc + (unsigned)(i * 23 + 3), 13u);
    }

    printf("%u %u\n", acc, axis3_w5_hfa_digest(last, 239u));
    return 0;
}
