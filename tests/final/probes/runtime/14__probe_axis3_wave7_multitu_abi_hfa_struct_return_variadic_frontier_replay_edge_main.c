#include <stdio.h>

struct Axis3W7Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W7Hfa axis3_w7_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w7_hfa_digest(struct Axis3W7Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W7Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 11; ++i) {
        last = axis3_w7_hfa_collect(
            1.0f + (float)i * 0.140625f,
            (unsigned)(47 + i * 13),
            (unsigned)(23 + i * 9),
            (unsigned)(11 + i * 5),
            13,
            0, 0.625 + (double)i * 0.25,
            1, 13 + i * 2,
            2, 53u + (unsigned)i * 11u,
            0, 1.5 + (double)i * 0.125,
            1, 29 - i,
            2, 71u + (unsigned)i * 9u,
            0, 2.375 + (double)i * 0.0625,
            1, 37 + i,
            2, 89u + (unsigned)i * 7u,
            0, 3.25 + (double)i * 0.03125,
            1, 49 + i * 3,
            2, 107u + (unsigned)i * 5u,
            1, 61 + i
        );

        acc ^= axis3_w7_hfa_digest(last, (unsigned)(i * 41 + 13));
        acc = rotl32(acc + (unsigned)(i * 31 + 7), 7u);
    }

    printf("%u %u\n", acc, axis3_w7_hfa_digest(last, 307u));
    return 0;
}
