#include <stdio.h>

struct Axis3W34Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W34Hfa axis3_w34_hfa_collect(
    float seed,
    unsigned frontier,
    unsigned shadow,
    unsigned replay,
    unsigned checkpoint,
    int count,
    ...
);
unsigned axis3_w34_hfa_digest(struct Axis3W34Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x71a4d25bu;
    struct Axis3W34Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 41; ++i) {
        last = axis3_w34_hfa_collect(
            1.4453125f + (float)i * 0.0478515625f,
            (unsigned)(37 + i * 23),
            (unsigned)(91 + i * 17),
            (unsigned)(29 + i * 31),
            (unsigned)(149 + i * 13),
            27,
            2, 211u + (unsigned)i * 31u,
            0, 1.046875 + (double)i * 0.0703125,
            1, 87 + i * 5,
            0, 1.703125 + (double)i * 0.03515625,
            2, 269u + (unsigned)i * 29u,
            1, 111 - i,
            0, 2.28125 + (double)i * 0.017578125,
            2, 331u + (unsigned)i * 23u,
            1, 141 + i * 2,
            0, 2.765625 + (double)i * 0.0087890625,
            2, 389u + (unsigned)i * 19u,
            1, 173 + i,
            0, 3.15625 + (double)i * 0.00439453125,
            2, 443u + (unsigned)i * 17u,
            1, 201 + i * 3,
            0, 3.4921875 + (double)i * 0.002197265625,
            2, 499u + (unsigned)i * 13u,
            1, 229 - i,
            0, 3.78125 + (double)i * 0.0010986328125,
            2, 557u + (unsigned)i * 11u,
            1, 251 + i * 2,
            0, 4.015625 + (double)i * 0.00054931640625,
            2, 613u + (unsigned)i * 7u,
            1, 277 + i,
            0, 4.203125 + (double)i * 0.000274658203125,
            2, 659u + (unsigned)i * 5u,
            1, 299 + i * 2,
            0, 4.359375 + (double)i * 0.0001373291015625,
            2, 701u + (unsigned)i * 3u,
            1, 317 - i
        );

        acc ^= axis3_w34_hfa_digest(last, (unsigned)(i * 191 + 73));
        acc = rotl32(acc + (unsigned)(i * 127 + 59), 9u);
    }

    printf("%u %u\n", acc, axis3_w34_hfa_digest(last, 2221u));
    return 0;
}
