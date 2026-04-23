#include <stdio.h>

struct Axis3W5Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W5Wide axis3_w5_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned checkpoint,
    unsigned rotate,
    int count,
    ...
);
unsigned axis3_w5_wide_digest(struct Axis3W5Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W5Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 8; ++i) {
        last = axis3_w5_wide_collect(
            1.375L + (long double)i * 0.125L,
            1.125L + (long double)i * 0.0625L,
            0.875L + (long double)i * 0.03125L,
            (unsigned)(31 + i * 11),
            (unsigned)(i & 7),
            11,
            0, 0.625L + (long double)i * 0.25L,
            1, 11 + i * 2,
            2, 53u + (unsigned)i * 13u,
            0, 1.5L + (long double)i * 0.125L,
            1, 23 - i,
            2, 71u + (unsigned)i * 9u,
            0, 2.625L + (long double)i * 0.0625L,
            1, 37 + i,
            2, 89u + (unsigned)i * 7u,
            1, 49 + i * 3,
            2, 101u + (unsigned)i * 5u
        );

        acc ^= axis3_w5_wide_digest(last, (unsigned)(i * 47 + 9));
        acc = rotl32(acc + (unsigned)(i * 43 + 3), 13u);
    }

    printf("%u %u\n", acc, axis3_w5_wide_digest(last, 251u));
    return 0;
}
