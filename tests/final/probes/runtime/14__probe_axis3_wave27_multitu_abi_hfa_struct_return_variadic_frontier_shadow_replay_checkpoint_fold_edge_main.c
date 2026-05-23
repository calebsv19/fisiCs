#include <stdio.h>

struct Axis3W27Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W27Hfa axis3_w27_hfa_collect(
    float seed,
    unsigned frontier,
    unsigned shadow,
    unsigned replay,
    unsigned checkpoint,
    int count,
    ...
);
unsigned axis3_w27_hfa_digest(struct Axis3W27Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x4d6c2f91u;
    struct Axis3W27Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 34; ++i) {
        last = axis3_w27_hfa_collect(
            1.421875f + (float)i * 0.05078125f,
            (unsigned)(41 + i * 19),
            (unsigned)(67 + i * 23),
            (unsigned)(29 + i * 31),
            (unsigned)(173 + i * 13),
            23,
            2, 241u + (unsigned)i * 37u,
            0, 1.109375 + (double)i * 0.078125,
            1, 97 + i * 5,
            0, 1.796875 + (double)i * 0.0390625,
            2, 307u + (unsigned)i * 29u,
            1, 131 - i,
            0, 2.359375 + (double)i * 0.01953125,
            2, 353u + (unsigned)i * 23u,
            1, 149 + i * 2,
            0, 2.859375 + (double)i * 0.009765625,
            2, 401u + (unsigned)i * 17u,
            1, 181 + i,
            0, 3.234375 + (double)i * 0.0048828125,
            2, 449u + (unsigned)i * 13u,
            1, 211 + i * 3,
            0, 3.5625 + (double)i * 0.00244140625,
            2, 503u + (unsigned)i * 11u,
            1, 239 - i,
            0, 3.828125 + (double)i * 0.001220703125,
            2, 557u + (unsigned)i * 7u,
            1, 263 + i * 2,
            0, 4.046875 + (double)i * 0.0006103515625,
            2, 601u + (unsigned)i * 5u,
            1, 281 + i
        );

        acc ^= axis3_w27_hfa_digest(last, (unsigned)(i * 151 + 109));
        acc = rotl32(acc + (unsigned)(i * 97 + 33), 9u);
    }

    printf("%u %u\n", acc, axis3_w27_hfa_digest(last, 1601u));
    return 0;
}
