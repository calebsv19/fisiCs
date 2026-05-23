#include <stdio.h>

struct Axis3W22Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W22Hfa axis3_w22_hfa_collect(
    float seed,
    unsigned frontier,
    unsigned replay,
    unsigned handoff,
    int count,
    ...
);
unsigned axis3_w22_hfa_digest(struct Axis3W22Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    n &= 31u;
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> ((32u - n) & 31u));
}

int main(void) {
    unsigned acc = 0x4a6db137u;
    struct Axis3W22Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 28; ++i) {
        last = axis3_w22_hfa_collect(
            1.3359375f + (float)i * 0.064453125f,
            (unsigned)(173 + i * 43),
            (unsigned)(31 + i * 29),
            (unsigned)(47 + i * 17),
            25,
            2, 191u + (unsigned)i * 29u,
            0, 1.0625 + (double)i * 0.09765625,
            1, 69 + i * 7,
            0, 2.109375 + (double)i * 0.048828125,
            2, 233u + (unsigned)i * 23u,
            1, 87 - i,
            0, 2.8125 + (double)i * 0.0244140625,
            2, 281u + (unsigned)i * 19u,
            1, 111 + i * 2,
            0, 3.390625 + (double)i * 0.01220703125,
            2, 337u + (unsigned)i * 17u,
            1, 141 + i,
            0, 3.84375 + (double)i * 0.006103515625,
            2, 379u + (unsigned)i * 13u,
            1, 167 + i * 3,
            0, 4.203125 + (double)i * 0.0030517578125,
            2, 421u + (unsigned)i * 11u,
            1, 189 - i,
            0, 4.5 + (double)i * 0.00152587890625,
            2, 463u + (unsigned)i * 7u,
            1, 211 + i * 2,
            0, 4.734375 + (double)i * 0.000762939453125,
            2, 509u + (unsigned)i * 5u,
            1, 239 + i
        );

        acc ^= axis3_w22_hfa_digest(last, (unsigned)(i * 113 + 71));
        acc = rotl32(acc + (unsigned)(i * 83 + 29), 13u);
    }

    printf("%u %u\n", acc, axis3_w22_hfa_digest(last, 1237u));
    return 0;
}
