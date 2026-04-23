#include <stdio.h>

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned qld(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.25L) + 0.75L;
    }
    return (unsigned)(v * 512.0L + 0.5L);
}

static long double axis3_w1_leaf(
    long double a0,
    long double a1,
    long double a2,
    long double a3,
    long double a4,
    unsigned u0,
    unsigned u1,
    unsigned u2,
    unsigned u3,
    unsigned u4,
    unsigned u5,
    int s0,
    int s1,
    int s2,
    int s3
) {
    long double sum = a0 + a1 * 0.5L + a2 * 0.25L + a3 * 0.125L + a4 * 0.0625L;
    sum += (long double)(u0 + u2 + u4) * 0.015625L;
    sum += (long double)(u1 + u3 + u5) * 0.0078125L;
    sum += (long double)(s0 - s1 + s2 - s3) * 0.03125L;
    return sum;
}

static long double axis3_w1_step_one(
    long double seed,
    int round,
    unsigned a,
    unsigned b,
    unsigned c,
    unsigned d,
    unsigned e,
    unsigned f,
    unsigned g,
    unsigned h
) {
    return axis3_w1_leaf(
        seed,
        seed * 0.75L + (long double)round * 0.125L,
        seed * 0.5L + (long double)(a & 7u) * 0.0625L,
        seed * 0.25L + (long double)(b & 15u) * 0.03125L,
        seed * 0.125L + (long double)(c & 31u) * 0.015625L,
        a, b, c, d, e, f,
        round,
        (int)(g & 63u),
        (int)(h & 31u),
        (int)((a ^ e) & 15u)
    );
}

static long double axis3_w1_step_two(
    long double seed,
    unsigned p0,
    unsigned p1,
    unsigned p2,
    unsigned p3,
    unsigned p4,
    unsigned p5,
    unsigned p6,
    unsigned p7,
    unsigned p8,
    unsigned p9
) {
    long double x0 = axis3_w1_step_one(seed, (int)(p0 & 31u), p1, p2, p3, p4, p5, p6, p7, p8);
    long double x1 = axis3_w1_step_one(seed + x0 * 0.0625L, (int)(p9 & 31u), p8, p7, p6, p5, p4, p3, p2, p1);
    return x0 * 0.625L + x1 * 0.375L + (long double)(p0 ^ p9) * 0.03125L;
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    long double seed = 1.25L;
    int i;

    for (i = 0; i < 11; ++i) {
        long double out = axis3_w1_step_two(
            seed,
            (unsigned)(17 + i * 3),
            (unsigned)(23 + i * 5),
            (unsigned)(31 + i * 7),
            (unsigned)(43 + i * 11),
            (unsigned)(59 + i * 13),
            (unsigned)(71 + i * 17),
            (unsigned)(89 + i * 19),
            (unsigned)(97 + i * 23),
            (unsigned)(109 + i * 29),
            (unsigned)(127 + i * 31)
        );

        acc ^= qld(out) + (unsigned)(i * 41 + 9);
        acc = rotl32(acc, 7u);
        seed = out * 0.5L + seed * 0.375L + (long double)i * 0.03125L;
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
