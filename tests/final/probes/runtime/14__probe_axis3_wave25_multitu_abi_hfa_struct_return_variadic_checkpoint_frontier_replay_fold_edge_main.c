#include <stdio.h>

struct Axis3W25Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W25Hfa axis3_w25_hfa_collect(
    float seed,
    unsigned checkpoint,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w25_hfa_digest(struct Axis3W25Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x6e2574c1u;
    struct Axis3W25Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 31; ++i) {
        last = axis3_w25_hfa_collect(
            1.5078125f + (float)i * 0.056640625f,
            (unsigned)(59 + i * 23),
            (unsigned)(181 + i * 19),
            (unsigned)(43 + i * 17),
            23,
            1, 83 + i * 7,
            0, 1.03125 + (double)i * 0.0859375,
            2, 241u + (unsigned)i * 29u,
            0, 1.921875 + (double)i * 0.04296875,
            1, 107 - i,
            2, 293u + (unsigned)i * 23u,
            0, 2.53125 + (double)i * 0.021484375,
            1, 131 + i * 2,
            2, 347u + (unsigned)i * 19u,
            0, 3.046875 + (double)i * 0.0107421875,
            1, 157 + i,
            2, 397u + (unsigned)i * 17u,
            0, 3.453125 + (double)i * 0.00537109375,
            1, 181 + i * 3,
            2, 449u + (unsigned)i * 13u,
            0, 3.78125 + (double)i * 0.002685546875,
            1, 209 - i,
            2, 503u + (unsigned)i * 11u,
            0, 4.046875 + (double)i * 0.0013427734375,
            1, 233 + i * 2,
            2, 557u + (unsigned)i * 7u,
            0, 4.265625 + (double)i * 0.00067138671875,
            1, 263 + i
        );

        acc ^= axis3_w25_hfa_digest(last, (unsigned)(i * 137 + 83));
        acc = rotl32(acc + (unsigned)(i * 101 + 19), 11u);
    }

    printf("%u %u\n", acc, axis3_w25_hfa_digest(last, 1429u));
    return 0;
}
