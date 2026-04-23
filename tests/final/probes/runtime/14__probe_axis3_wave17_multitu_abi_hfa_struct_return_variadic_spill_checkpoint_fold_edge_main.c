#include <stdio.h>

struct Axis3W17Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W17Hfa axis3_w17_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w17_hfa_digest(struct Axis3W17Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W17Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 21; ++i) {
        last = axis3_w17_hfa_collect(
            1.0078125f + (float)i * 0.083984375f,
            (unsigned)(227 + i * 47),
            (unsigned)(21 + i * 33),
            (unsigned)(11 + i * 27),
            23,
            2, 101u + (unsigned)i * 37u,
            0, 0.875 + (double)i * 0.140625,
            1, 43 + i * 17,
            0, 2.125 + (double)i * 0.0703125,
            2, 211u + (unsigned)i * 23u,
            1, 67 - i,
            0, 3.0 + (double)i * 0.03515625,
            2, 263u + (unsigned)i * 19u,
            1, 89 + i,
            0, 3.65625 + (double)i * 0.017578125,
            1, 107 + i * 2,
            2, 307u + (unsigned)i * 17u,
            0, 4.21875 + (double)i * 0.0087890625,
            1, 131 + i,
            2, 347u + (unsigned)i * 13u,
            0, 4.6875 + (double)i * 0.00439453125,
            1, 157 + i * 3,
            2, 389u + (unsigned)i * 11u,
            0, 5.0 + (double)i * 0.002197265625,
            1, 179 - i,
            2, 431u + (unsigned)i * 7u,
            0, 5.25 + (double)i * 0.0010986328125,
            1, 199 + i * 2
        );

        acc ^= axis3_w17_hfa_digest(last, (unsigned)(i * 89 + 61));
        acc = rotl32(acc + (unsigned)(i * 79 + 41), 11u);
    }

    printf("%u %u\n", acc, axis3_w17_hfa_digest(last, 887u));
    return 0;
}
