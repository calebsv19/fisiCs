#include <stdio.h>

struct Axis3W10Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W10Wide axis3_w10_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w10_wide_digest(struct Axis3W10Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W10Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 12; ++i) {
        last = axis3_w10_wide_collect(
            1.3125L + (long double)i * 0.140625L,
            1.0625L + (long double)i * 0.0703125L,
            0.8125L + (long double)i * 0.0390625L,
            (unsigned)(83 + i * 19),
            (unsigned)(7 + i * 15),
            (unsigned)(1 + i * 11),
            16,
            1, 29 + i * 7,
            0, 1.25L + (long double)i * 0.1484375L,
            2, 101u + (unsigned)i * 17u,
            0, 2.125L + (long double)i * 0.07421875L,
            1, 43 - i,
            2, 131u + (unsigned)i * 13u,
            0, 3.1875L + (long double)i * 0.037109375L,
            1, 61 + i,
            2, 167u + (unsigned)i * 11u,
            0, 3.875L + (long double)i * 0.0185546875L,
            2, 197u + (unsigned)i * 9u,
            1, 79 + i * 2,
            0, 4.4375L + (long double)i * 0.00927734375L,
            1, 97 + i,
            2, 229u + (unsigned)i * 7u,
            0, 4.875L + (long double)i * 0.004638671875L,
            1, 113 + i * 3
        );

        acc ^= axis3_w10_wide_digest(last, (unsigned)(i * 71 + 31));
        acc = rotl32(acc + (unsigned)(i * 53 + 17), 9u);
    }

    printf("%u %u\n", acc, axis3_w10_wide_digest(last, 521u));
    return 0;
}
