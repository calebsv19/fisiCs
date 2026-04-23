#include <stdio.h>

struct Axis3W4Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W4Wide axis3_w4_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned seed_tag,
    unsigned seed_mode,
    int count,
    ...
);
unsigned axis3_w4_wide_digest(struct Axis3W4Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W4Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 8; ++i) {
        last = axis3_w4_wide_collect(
            1.25L + (long double)i * 0.125L,
            1.0L + (long double)i * 0.0625L,
            0.75L + (long double)i * 0.03125L,
            (unsigned)(23 + i * 9),
            (unsigned)(i & 7),
            12,
            0, 0.5L + (long double)i * 0.25L,
            1, 9 + i * 2,
            2, 47u + (unsigned)i * 13u,
            0, 1.375L + (long double)i * 0.125L,
            1, 19 - i,
            2, 61u + (unsigned)i * 11u,
            0, 2.5L + (long double)i * 0.0625L,
            1, 31 + i,
            2, 79u + (unsigned)i * 7u,
            0, 3.125L + (long double)i * 0.03125L,
            1, 43 + i * 3,
            2, 97u + (unsigned)i * 5u
        );

        acc ^= axis3_w4_wide_digest(last, (unsigned)(i * 41 + 7));
        acc = rotl32(acc + (unsigned)(i * 37 + 11), 11u);
    }

    printf("%u %u\n", acc, axis3_w4_wide_digest(last, 223u));
    return 0;
}
