#include <stdio.h>

struct Axis3W7Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W7Wide axis3_w7_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w7_wide_digest(struct Axis3W7Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W7Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 9; ++i) {
        last = axis3_w7_wide_collect(
            1.625L + (long double)i * 0.125L,
            1.375L + (long double)i * 0.0625L,
            1.125L + (long double)i * 0.03125L,
            (unsigned)(53 + i * 11),
            (unsigned)(29 + i * 7),
            (unsigned)(13 + i * 5),
            13,
            0, 0.875L + (long double)i * 0.25L,
            1, 13 + i * 2,
            2, 67u + (unsigned)i * 13u,
            0, 1.75L + (long double)i * 0.125L,
            1, 27 - i,
            2, 89u + (unsigned)i * 9u,
            0, 2.875L + (long double)i * 0.0625L,
            1, 41 + i,
            2, 109u + (unsigned)i * 7u,
            0, 3.5L + (long double)i * 0.03125L,
            1, 53 + i * 3,
            2, 127u + (unsigned)i * 5u,
            1, 67 + i
        );

        acc ^= axis3_w7_wide_digest(last, (unsigned)(i * 47 + 11));
        acc = rotl32(acc + (unsigned)(i * 43 + 5), 13u);
    }

    printf("%u %u\n", acc, axis3_w7_wide_digest(last, 313u));
    return 0;
}
