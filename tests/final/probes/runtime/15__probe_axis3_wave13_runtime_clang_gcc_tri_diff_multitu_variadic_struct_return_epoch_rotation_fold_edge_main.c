#include <stdio.h>

struct Axis3W13Wide {
    long double lo;
    long double mid;
    long double hi;
    unsigned tag;
};

struct Axis3W13Wide axis3_w13_wide_collect(
    long double seed_lo,
    long double seed_mid,
    long double seed_hi,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int count,
    ...
);
unsigned axis3_w13_wide_digest(struct Axis3W13Wide v, unsigned salt);

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    struct Axis3W13Wide last;
    int i;

    last.lo = 0.0L;
    last.mid = 0.0L;
    last.hi = 0.0L;
    last.tag = 0u;

    for (i = 0; i < 14; ++i) {
        last = axis3_w13_wide_collect(
            1.125L + (long double)i * 0.12109375L,
            0.90625L + (long double)i * 0.060546875L,
            0.671875L + (long double)i * 0.0322265625L,
            (unsigned)(137 + i * 23),
            (unsigned)(7 + i * 19),
            (unsigned)(3 + i * 17),
            19,
            2, 71u + (unsigned)i * 23u,
            0, 1.28125L + (long double)i * 0.12890625L,
            1, 35 + i * 11,
            0, 2.1875L + (long double)i * 0.064453125L,
            2, 163u + (unsigned)i * 17u,
            1, 51 - i,
            0, 3.28125L + (long double)i * 0.0322265625L,
            1, 69 + i,
            2, 211u + (unsigned)i * 13u,
            0, 3.90625L + (long double)i * 0.01611328125L,
            1, 87 + i * 2,
            2, 241u + (unsigned)i * 11u,
            0, 4.4375L + (long double)i * 0.008056640625L,
            2, 271u + (unsigned)i * 9u,
            1, 103 + i,
            0, 4.875L + (long double)i * 0.0040283203125L,
            1, 129 + i * 3,
            2, 307u + (unsigned)i * 7u,
            0, 5.1875L + (long double)i * 0.00201416015625L
        );

        acc ^= axis3_w13_wide_digest(last, (unsigned)(i * 83 + 43));
        acc = rotl32(acc + (unsigned)(i * 61 + 23), 7u);
    }

    printf("%u %u\n", acc, axis3_w13_wide_digest(last, 641u));
    return 0;
}
