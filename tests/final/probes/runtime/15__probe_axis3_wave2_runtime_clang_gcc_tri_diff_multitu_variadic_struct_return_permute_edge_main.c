#include <stdio.h>

struct Axis3W2Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W2Wide axis3_w2_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned seed_tag,
    int count,
    ...
);
unsigned axis3_w2_wide_digest(struct Axis3W2Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W2Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 8; ++i) {
        last = axis3_w2_wide_collect(
            1.0L + (long double)i * 0.125L,
            0.75L + (long double)i * 0.0625L,
            0.5L + (long double)i * 0.03125L,
            (unsigned)(17 + i * 5),
            9,
            0, 0.5L + (long double)i * 0.25L,
            1, 7 + i * 3,
            0, 1.125L + (long double)i * 0.125L,
            1, 19 - i,
            0, 2.0L + (long double)i * 0.0625L,
            1, 29 + i * 2,
            0, 3.25L + (long double)i * 0.03125L,
            1, 37 + i
        );

        acc ^= axis3_w2_wide_digest(last, (unsigned)(i * 23 + 11));
        acc = rotl32(acc + (unsigned)(i * 31 + 3), 11u);
    }

    printf("%u %u\n", acc, axis3_w2_wide_digest(last, 251u));
    return 0;
}
