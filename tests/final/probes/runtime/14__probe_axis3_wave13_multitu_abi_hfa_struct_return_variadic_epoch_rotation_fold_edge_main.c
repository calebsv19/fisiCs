#include <stdio.h>

struct Axis3W13Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W13Hfa axis3_w13_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w13_hfa_digest(struct Axis3W13Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W13Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 16; ++i) {
        last = axis3_w13_hfa_collect(
            1.15625f + (float)i * 0.09765625f,
            (unsigned)(131 + i * 31),
            (unsigned)(9 + i * 21),
            (unsigned)(3 + i * 19),
            19,
            2, 61u + (unsigned)i * 29u,
            0, 0.9375 + (double)i * 0.1328125,
            1, 33 + i * 11,
            0, 2.1875 + (double)i * 0.06640625,
            2, 149u + (unsigned)i * 17u,
            1, 57 - i,
            0, 3.0625 + (double)i * 0.033203125,
            2, 197u + (unsigned)i * 13u,
            1, 77 + i,
            0, 3.71875 + (double)i * 0.0166015625,
            1, 95 + i * 2,
            2, 233u + (unsigned)i * 11u,
            0, 4.28125 + (double)i * 0.00830078125,
            1, 113 + i,
            2, 263u + (unsigned)i * 9u,
            0, 4.75 + (double)i * 0.004150390625,
            1, 129 + i * 3,
            2, 289u + (unsigned)i * 7u,
            0, 5.0625 + (double)i * 0.0020751953125
        );

        acc ^= axis3_w13_hfa_digest(last, (unsigned)(i * 67 + 41));
        acc = rotl32(acc + (unsigned)(i * 53 + 23), 9u);
    }

    printf("%u %u\n", acc, axis3_w13_hfa_digest(last, 601u));
    return 0;
}
