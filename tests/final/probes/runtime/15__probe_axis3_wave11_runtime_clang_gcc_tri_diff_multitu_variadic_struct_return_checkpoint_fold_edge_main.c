#include <stdio.h>

struct Axis3W11Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W11Wide axis3_w11_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w11_wide_digest(struct Axis3W11Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W11Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 13; ++i) {
        last = axis3_w11_wide_collect(
            1.1875L + (long double)i * 0.1328125L,
            0.9375L + (long double)i * 0.06640625L,
            0.6875L + (long double)i * 0.03515625L,
            (unsigned)(101 + i * 23),
            (unsigned)(3 + i * 17),
            (unsigned)(1 + i * 13),
            17,
            2, 53u + (unsigned)i * 19u,
            0, 1.3125L + (long double)i * 0.140625L,
            1, 31 + i * 9,
            0, 2.25L + (long double)i * 0.0703125L,
            2, 139u + (unsigned)i * 17u,
            1, 47 - i,
            0, 3.3125L + (long double)i * 0.03515625L,
            1, 67 + i,
            2, 179u + (unsigned)i * 13u,
            0, 3.9375L + (long double)i * 0.017578125L,
            1, 83 + i * 2,
            2, 211u + (unsigned)i * 11u,
            0, 4.5L + (long double)i * 0.0087890625L,
            2, 241u + (unsigned)i * 9u,
            1, 101 + i,
            0, 4.9375L + (long double)i * 0.00439453125L,
            1, 127 + i * 3,
            2, 271u + (unsigned)i * 7u
        );

        acc ^= axis3_w11_wide_digest(last, (unsigned)(i * 79 + 37));
        acc = rotl32(acc + (unsigned)(i * 59 + 19), 7u);
    }

    printf("%u %u\n", acc, axis3_w11_wide_digest(last, 593u));
    return 0;
}
