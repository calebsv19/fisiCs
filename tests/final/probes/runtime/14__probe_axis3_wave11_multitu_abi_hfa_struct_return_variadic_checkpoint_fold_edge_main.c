#include <stdio.h>

struct Axis3W11Hfa {
    float a;
    float b;
    float c;
    float d;
};

struct Axis3W11Hfa axis3_w11_hfa_collect(
    float seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w11_hfa_digest(struct Axis3W11Hfa v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W11Hfa last = {0.0f, 0.0f, 0.0f, 0.0f};
    int i;

    for (i = 0; i < 15; ++i) {
        last = axis3_w11_hfa_collect(
            1.1875f + (float)i * 0.109375f,
            (unsigned)(97 + i * 29),
            (unsigned)(5 + i * 19),
            (unsigned)(1 + i * 17),
            17,
            2, 47u + (unsigned)i * 23u,
            0, 1.0 + (double)i * 0.1484375,
            1, 27 + i * 9,
            0, 2.0625 + (double)i * 0.07421875,
            2, 113u + (unsigned)i * 17u,
            1, 49 - i,
            0, 2.9375 + (double)i * 0.037109375,
            2, 157u + (unsigned)i * 13u,
            1, 71 + i,
            0, 3.6875 + (double)i * 0.0185546875,
            1, 89 + i * 2,
            2, 193u + (unsigned)i * 11u,
            0, 4.3125 + (double)i * 0.00927734375,
            1, 107 + i,
            2, 227u + (unsigned)i * 9u,
            0, 4.8125 + (double)i * 0.004638671875,
            1, 127 + i * 3,
            2, 251u + (unsigned)i * 7u
        );

        acc ^= axis3_w11_hfa_digest(last, (unsigned)(i * 59 + 31));
        acc = rotl32(acc + (unsigned)(i * 47 + 17), 13u);
    }

    printf("%u %u\n", acc, axis3_w11_hfa_digest(last, 557u));
    return 0;
}
