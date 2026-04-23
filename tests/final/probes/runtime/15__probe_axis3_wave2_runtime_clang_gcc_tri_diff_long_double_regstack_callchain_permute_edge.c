#include <stdio.h>

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned qld(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.375L) + 0.5L;
    }
    return (unsigned)(v * 1024.0L + 0.5L);
}

static long double axis3_w2_leaf(
    long double a0,
    long double a1,
    long double a2,
    long double a3,
    long double a4,
    long double a5,
    unsigned u0,
    unsigned u1,
    unsigned u2,
    unsigned u3,
    unsigned u4,
    unsigned u5,
    unsigned u6,
    unsigned u7,
    int s0,
    int s1,
    int s2,
    int s3,
    int s4
) {
    long double sum = a0 + a1 * 0.5L + a2 * 0.25L + a3 * 0.125L;
    sum += a4 * 0.0625L + a5 * 0.03125L;
    sum += (long double)(u0 + u2 + u4 + u6) * 0.015625L;
    sum += (long double)(u1 + u3 + u5 + u7) * 0.0078125L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4) * 0.03125L;
    return sum;
}

static long double axis3_w2_stage_one(
    long double seed,
    int round,
    unsigned p0,
    unsigned p1,
    unsigned p2,
    unsigned p3,
    unsigned p4,
    unsigned p5,
    unsigned p6,
    unsigned p7
) {
    return axis3_w2_leaf(
        seed,
        seed * 0.75L + (long double)round * 0.125L,
        seed * 0.5L + (long double)(p0 & 31u) * 0.0625L,
        seed * 0.25L + (long double)(p1 & 31u) * 0.03125L,
        seed * 0.125L + (long double)(p2 & 15u) * 0.015625L,
        seed * 0.0625L + (long double)(p3 & 15u) * 0.0078125L,
        p0, p1, p2, p3, p4, p5, p6, p7,
        round,
        (int)(p4 & 63u),
        (int)(p5 & 31u),
        (int)(p6 & 15u),
        (int)(p7 & 7u)
    );
}

static long double axis3_w2_stage_two(
    long double seed,
    unsigned a0,
    unsigned a1,
    unsigned a2,
    unsigned a3,
    unsigned a4,
    unsigned a5,
    unsigned a6,
    unsigned a7,
    unsigned a8,
    unsigned a9
) {
    long double x0 = axis3_w2_stage_one(seed, (int)(a0 & 31u), a1, a2, a3, a4, a5, a6, a7, a8);
    long double x1 = axis3_w2_stage_one(
        seed + x0 * 0.0625L,
        (int)(a9 & 31u),
        a8, a7, a6, a5, a4, a3, a2, a1
    );
    long double x2 = axis3_w2_stage_one(
        seed + x1 * 0.03125L,
        (int)((a3 ^ a6) & 31u),
        a2, a4, a6, a8, a1, a3, a5, a7
    );
    return x0 * 0.5L + x1 * 0.3125L + x2 * 0.1875L + (long double)(a0 ^ a9) * 0.03125L;
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    long double seed = 1.375L;
    int i;

    for (i = 0; i < 12; ++i) {
        long double out = axis3_w2_stage_two(
            seed,
            (unsigned)(19 + i * 3),
            (unsigned)(29 + i * 5),
            (unsigned)(37 + i * 7),
            (unsigned)(43 + i * 11),
            (unsigned)(53 + i * 13),
            (unsigned)(61 + i * 17),
            (unsigned)(71 + i * 19),
            (unsigned)(83 + i * 23),
            (unsigned)(97 + i * 29),
            (unsigned)(109 + i * 31)
        );

        acc ^= qld(out) + (unsigned)(i * 47 + 9);
        acc = rotl32(acc, 9u);
        seed = out * 0.4375L + seed * 0.5L + (long double)i * 0.015625L;
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
