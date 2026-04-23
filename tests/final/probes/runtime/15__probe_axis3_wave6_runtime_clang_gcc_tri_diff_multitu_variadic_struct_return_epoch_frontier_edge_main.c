#include <stdio.h>

struct Axis3W6Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W6Wide axis3_w6_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    int count,
    ...
);
unsigned axis3_w6_wide_digest(struct Axis3W6Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W6Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 9; ++i) {
        last = axis3_w6_wide_collect(
            1.5L + (long double)i * 0.125L,
            1.25L + (long double)i * 0.0625L,
            1.0L + (long double)i * 0.03125L,
            (unsigned)(43 + i * 11),
            (unsigned)(19 + i * 7),
            12,
            0, 0.75L + (long double)i * 0.25L,
            1, 11 + i * 2,
            2, 59u + (unsigned)i * 13u,
            0, 1.625L + (long double)i * 0.125L,
            1, 23 - i,
            2, 79u + (unsigned)i * 9u,
            0, 2.75L + (long double)i * 0.0625L,
            1, 37 + i,
            2, 97u + (unsigned)i * 7u,
            0, 3.25L + (long double)i * 0.03125L,
            1, 49 + i * 3,
            2, 113u + (unsigned)i * 5u
        );

        acc ^= axis3_w6_wide_digest(last, (unsigned)(i * 43 + 13));
        acc = rotl32(acc + (unsigned)(i * 41 + 7), 11u);
    }

    printf("%u %u\n", acc, axis3_w6_wide_digest(last, 271u));
    return 0;
}
