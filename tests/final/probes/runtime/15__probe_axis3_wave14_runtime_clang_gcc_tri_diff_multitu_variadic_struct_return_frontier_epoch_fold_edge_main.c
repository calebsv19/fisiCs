#include <stdio.h>

struct Axis3W14Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W14Wide axis3_w14_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w14_wide_digest(struct Axis3W14Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W14Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 15; ++i) {
        last = axis3_w14_wide_collect(
            1.09375L + (long double)i * 0.11328125L,
            0.875L + (long double)i * 0.056640625L,
            0.640625L + (long double)i * 0.0302734375L,
            (unsigned)(163 + i * 29),
            (unsigned)(9 + i * 21),
            (unsigned)(5 + i * 19),
            21,
            2, 89u + (unsigned)i * 29u,
            0, 1.25L + (long double)i * 0.12109375L,
            1, 39 + i * 13,
            0, 2.15625L + (long double)i * 0.060546875L,
            2, 191u + (unsigned)i * 19u,
            1, 59 - i,
            0, 3.25L + (long double)i * 0.0302734375L,
            1, 77 + i,
            2, 239u + (unsigned)i * 17u,
            0, 3.875L + (long double)i * 0.01513671875L,
            1, 93 + i * 2,
            2, 271u + (unsigned)i * 13u,
            0, 4.40625L + (long double)i * 0.007568359375L,
            2, 307u + (unsigned)i * 11u,
            1, 111 + i,
            0, 4.84375L + (long double)i * 0.0037841796875L,
            1, 137 + i * 3,
            2, 349u + (unsigned)i * 7u,
            0, 5.15625L + (long double)i * 0.00189208984375L,
            1, 173 - i,
            2, 379u + (unsigned)i * 5u
        );

        acc ^= axis3_w14_wide_digest(last, (unsigned)(i * 89 + 47));
        acc = rotl32(acc + (unsigned)(i * 67 + 29), 9u);
    }

    printf("%u %u\n", acc, axis3_w14_wide_digest(last, 709u));
    return 0;
}
