#include <stdio.h>

struct Axis3W10Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W10Hfa axis3_w10_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w10_hfa_digest(struct Axis3W10Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W10Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 14; ++i) {
        last = axis3_w10_hfa_collect(
            1.0625f + (float)i * 0.1171875f,
            (unsigned)(79 + i * 23),
            (unsigned)(9 + i * 17),
            (unsigned)(3 + i * 13),
            16,
            1, 25 + i * 7,
            0, 0.9375 + (double)i * 0.15625,
            2, 53u + (unsigned)i * 19u,
            0, 1.9375 + (double)i * 0.078125,
            1, 41 - i,
            2, 97u + (unsigned)i * 15u,
            0, 2.8125 + (double)i * 0.0390625,
            1, 59 + i,
            2, 139u + (unsigned)i * 11u,
            0, 3.5625 + (double)i * 0.01953125,
            2, 173u + (unsigned)i * 9u,
            1, 77 + i * 2,
            0, 4.1875 + (double)i * 0.009765625,
            1, 95 + i,
            2, 211u + (unsigned)i * 7u,
            0, 4.75 + (double)i * 0.0048828125,
            1, 113 + i * 3
        );

        acc ^= axis3_w10_hfa_digest(last, (unsigned)(i * 53 + 29));
        acc = rotl32(acc + (unsigned)(i * 43 + 13), 11u);
    }

    printf("%u %u\n", acc, axis3_w10_hfa_digest(last, 487u));
    return 0;
}
