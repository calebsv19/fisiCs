#include <stdio.h>

struct Axis3W3Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W3Wide axis3_w3_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned seed_tag,
    int count,
    ...
);
unsigned axis3_w3_wide_digest(struct Axis3W3Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W3Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 9; ++i) {
        last = axis3_w3_wide_collect(
            1.125L + (long double)i * 0.125L,
            0.875L + (long double)i * 0.0625L,
            0.625L + (long double)i * 0.03125L,
            (unsigned)(19 + i * 7),
            10,
            0, 0.75L + (long double)i * 0.25L,
            1, 7 + i * 3,
            2, 41u + (unsigned)i * 11u,
            0, 1.375L + (long double)i * 0.125L,
            1, 17 - i,
            2, 59u + (unsigned)i * 9u,
            0, 2.25L + (long double)i * 0.0625L,
            1, 29 + i,
            2, 73u + (unsigned)i * 5u,
            1, 43 + i * 2
        );

        acc ^= axis3_w3_wide_digest(last, (unsigned)(i * 29 + 13));
        acc = rotl32(acc + (unsigned)(i * 31 + 5), 9u);
    }

    printf("%u %u\n", acc, axis3_w3_wide_digest(last, 197u));
    return 0;
}
