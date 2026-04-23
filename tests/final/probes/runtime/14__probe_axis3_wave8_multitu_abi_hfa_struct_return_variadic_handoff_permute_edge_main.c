#include <stdio.h>

struct Axis3W8Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W8Hfa axis3_w8_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w8_hfa_digest(struct Axis3W8Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W8Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 12; ++i) {
        last = axis3_w8_hfa_collect(
            0.875f + (float)i * 0.15625f,
            (unsigned)(61 + i * 17),
            (unsigned)(19 + i * 11),
            (unsigned)(7 + i * 9),
            15,
            0, 0.75 + (double)i * 0.1875,
            2, 41u + (unsigned)i * 13u,
            1, 17 + i * 3,
            0, 1.625 + (double)i * 0.09375,
            1, 33 - i,
            2, 73u + (unsigned)i * 9u,
            0, 2.5 + (double)i * 0.046875,
            2, 97u + (unsigned)i * 7u,
            1, 45 + i,
            0, 3.375 + (double)i * 0.0234375,
            1, 59 + i * 2,
            2, 131u + (unsigned)i * 5u,
            0, 4.0 + (double)i * 0.01171875,
            2, 151u + (unsigned)i * 3u,
            1, 71 + i
        );

        acc ^= axis3_w8_hfa_digest(last, (unsigned)(i * 43 + 17));
        acc = rotl32(acc + (unsigned)(i * 37 + 9), 9u);
    }

    printf("%u %u\n", acc, axis3_w8_hfa_digest(last, 389u));
    return 0;
}
