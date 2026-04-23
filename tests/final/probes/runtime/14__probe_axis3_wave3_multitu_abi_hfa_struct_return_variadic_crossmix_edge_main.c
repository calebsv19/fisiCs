#include <stdio.h>

struct Axis3W3Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W3Hfa axis3_w3_hfa_collect(float seed, int count, ...);
unsigned axis3_w3_hfa_digest(struct Axis3W3Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W3Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 11; ++i) {
        last = axis3_w3_hfa_collect(
            0.625f + (float)i * 0.15625f,
            9,
            0, 0.75 + (double)i * 0.125,
            1, 5 + i * 2,
            2, 17u + (unsigned)i * 7u,
            0, 1.125 + (double)i * 0.0625,
            1, 11 - i,
            2, 29u + (unsigned)i * 5u,
            0, 2.25 + (double)i * 0.03125,
            1, 19 + i,
            2, 43u + (unsigned)i * 3u
        );

        acc ^= axis3_w3_hfa_digest(last, (unsigned)(i * 31 + 9));
        acc = rotl32(acc + (unsigned)(i * 17 + 13), 9u);
    }

    printf("%u %u\n", acc, axis3_w3_hfa_digest(last, 173u));
    return 0;
}
