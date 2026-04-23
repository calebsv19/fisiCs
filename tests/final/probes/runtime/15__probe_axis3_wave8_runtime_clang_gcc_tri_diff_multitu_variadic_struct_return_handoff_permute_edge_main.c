#include <stdio.h>

struct Axis3W8Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W8Wide axis3_w8_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w8_wide_digest(struct Axis3W8Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W8Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 10; ++i) {
        last = axis3_w8_wide_collect(
            1.5L + (long double)i * 0.15625L,
            1.25L + (long double)i * 0.078125L,
            1.0L + (long double)i * 0.046875L,
            (unsigned)(59 + i * 13),
            (unsigned)(17 + i * 9),
            (unsigned)(5 + i * 7),
            15,
            0, 1.0L + (long double)i * 0.1875L,
            2, 71u + (unsigned)i * 11u,
            1, 19 + i * 3,
            0, 1.875L + (long double)i * 0.09375L,
            1, 31 - i,
            2, 97u + (unsigned)i * 9u,
            0, 3.0L + (long double)i * 0.046875L,
            2, 113u + (unsigned)i * 7u,
            1, 43 + i,
            0, 3.75L + (long double)i * 0.0234375L,
            1, 61 + i * 2,
            2, 149u + (unsigned)i * 5u,
            0, 4.25L + (long double)i * 0.01171875L,
            2, 173u + (unsigned)i * 3u,
            1, 79 + i
        );

        acc ^= axis3_w8_wide_digest(last, (unsigned)(i * 59 + 17));
        acc = rotl32(acc + (unsigned)(i * 41 + 9), 11u);
    }

    printf("%u %u\n", acc, axis3_w8_wide_digest(last, 397u));
    return 0;
}
