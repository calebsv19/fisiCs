#include <stdio.h>

struct Axis3W9Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W9Wide axis3_w9_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w9_wide_digest(struct Axis3W9Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W9Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 11; ++i) {
        last = axis3_w9_wide_collect(
            1.4375L + (long double)i * 0.1484375L,
            1.1875L + (long double)i * 0.07421875L,
            0.9375L + (long double)i * 0.04296875L,
            (unsigned)(71 + i * 17),
            (unsigned)(11 + i * 13),
            (unsigned)(3 + i * 9),
            15,
            2, 83u + (unsigned)i * 13u,
            0, 1.125L + (long double)i * 0.1640625L,
            1, 23 + i * 5,
            0, 2.0L + (long double)i * 0.08203125L,
            1, 37 - i,
            2, 109u + (unsigned)i * 11u,
            0, 3.125L + (long double)i * 0.041015625L,
            2, 137u + (unsigned)i * 9u,
            1, 49 + i,
            0, 3.8125L + (long double)i * 0.0205078125L,
            1, 67 + i * 2,
            2, 167u + (unsigned)i * 7u,
            0, 4.375L + (long double)i * 0.01025390625L,
            1, 89 + i,
            2, 191u + (unsigned)i * 5u
        );

        acc ^= axis3_w9_wide_digest(last, (unsigned)(i * 67 + 29));
        acc = rotl32(acc + (unsigned)(i * 47 + 13), 13u);
    }

    printf("%u %u\n", acc, axis3_w9_wide_digest(last, 463u));
    return 0;
}
