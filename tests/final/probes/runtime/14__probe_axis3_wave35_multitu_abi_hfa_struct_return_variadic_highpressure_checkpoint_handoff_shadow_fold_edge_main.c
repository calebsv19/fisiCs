#include <stdio.h>

struct Axis3W35Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W35Hfa axis3_w35_hfa_collect(
    float seed,
    unsigned checkpoint,
    unsigned handoff,
    unsigned shadow,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w35_hfa_digest(struct Axis3W35Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x6c1fa439u;
    struct Axis3W35Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 42; ++i) {
        last = axis3_w35_hfa_collect(
            1.4765625f + (float)i * 0.04833984375f,
            (unsigned)(151 + i * 13),
            (unsigned)(23 + i * 29),
            (unsigned)(97 + i * 17),
            (unsigned)(31 + i * 31),
            27,
            2, 223u + (unsigned)i * 31u,
            0, 1.078125 + (double)i * 0.0712890625,
            1, 93 + i * 5,
            0, 1.7421875 + (double)i * 0.03564453125,
            2, 281u + (unsigned)i * 29u,
            1, 117 - i,
            0, 2.3203125 + (double)i * 0.017822265625,
            2, 337u + (unsigned)i * 23u,
            1, 147 + i * 2,
            0, 2.8046875 + (double)i * 0.0089111328125,
            2, 397u + (unsigned)i * 19u,
            1, 177 + i,
            0, 3.1953125 + (double)i * 0.00445556640625,
            2, 449u + (unsigned)i * 17u,
            1, 207 + i * 3,
            0, 3.53125 + (double)i * 0.002227783203125,
            2, 503u + (unsigned)i * 13u,
            1, 233 - i,
            0, 3.8203125 + (double)i * 0.0011138916015625,
            2, 559u + (unsigned)i * 11u,
            1, 259 + i * 2,
            0, 4.0546875 + (double)i * 0.00055694580078125,
            2, 613u + (unsigned)i * 7u,
            1, 281 + i,
            0, 4.2421875 + (double)i * 0.000278472900390625,
            2, 659u + (unsigned)i * 5u,
            1, 303 + i * 2,
            0, 4.3984375 + (double)i * 0.0001392364501953125,
            2, 701u + (unsigned)i * 3u,
            1, 321 - i
        );

        acc ^= axis3_w35_hfa_digest(last, (unsigned)(i * 197 + 79));
        acc = rotl32(acc + (unsigned)(i * 131 + 53), 7u);
    }

    printf("%u %u\n", acc, axis3_w35_hfa_digest(last, 2281u));
    return 0;
}
