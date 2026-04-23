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

static long double axis3_w4_leaf(
    long double a0,
    long double a1,
    long double a2,
    long double a3,
    long double a4,
    long double a5,
    long double a6,
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
    int s4,
    int s5
) {
    long double sum = a0 + a1 * 0.5L + a2 * 0.25L + a3 * 0.125L;
    sum += a4 * 0.0625L + a5 * 0.03125L + a6 * 0.015625L;
    sum += (long double)(u0 + u2 + u4 + u6) * 0.015625L;
    sum += (long double)(u1 + u3 + u5 + u7) * 0.0078125L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4 - s5) * 0.03125L;
    return sum;
}

static long double axis3_w4_stage(long double seed, int round, unsigned p0, unsigned p1, unsigned p2, unsigned p3, unsigned p4, unsigned p5, unsigned p6, unsigned p7) {
    return axis3_w4_leaf(
        seed,
        seed * 0.75L + (long double)round * 0.125L,
        seed * 0.5L + (long double)(p0 & 31u) * 0.0625L,
        seed * 0.25L + (long double)(p1 & 31u) * 0.03125L,
        seed * 0.125L + (long double)(p2 & 15u) * 0.015625L,
        seed * 0.0625L + (long double)(p3 & 15u) * 0.0078125L,
        seed * 0.03125L + (long double)(p4 & 7u) * 0.00390625L,
        p0, p1, p2, p3, p4, p5, p6, p7,
        round,
        (int)(p2 & 63u),
        (int)(p3 & 31u),
        (int)(p4 & 15u),
        (int)(p5 & 15u),
        (int)(p6 & 7u)
    );
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    long double seed = 1.375L;
    int i;

    for (i = 0; i < 11; ++i) {
        long double x0 = axis3_w4_stage(seed, i, (unsigned)(19 + i * 3), (unsigned)(29 + i * 5), (unsigned)(37 + i * 7), (unsigned)(43 + i * 11), (unsigned)(53 + i * 13), (unsigned)(61 + i * 17), (unsigned)(71 + i * 19), (unsigned)(83 + i * 23));
        long double x1 = axis3_w4_stage(seed + x0 * 0.0625L, i + 5, (unsigned)(97 + i * 29), (unsigned)(109 + i * 31), (unsigned)(127 + i * 5), (unsigned)(149 + i * 7), (unsigned)(173 + i * 11), (unsigned)(197 + i * 13), (unsigned)(223 + i * 17), (unsigned)(251 + i * 19));
        long double x2 = axis3_w4_stage(seed + x1 * 0.03125L, i + 7, (unsigned)(269 + i * 3), (unsigned)(281 + i * 5), (unsigned)(307 + i * 7), (unsigned)(331 + i * 11), (unsigned)(347 + i * 13), (unsigned)(373 + i * 17), (unsigned)(389 + i * 19), (unsigned)(401 + i * 23));
        long double out = x0 * 0.5L + x1 * 0.3125L + x2 * 0.1875L + (long double)(i * 3) * 0.015625L;

        acc ^= qld(out) + (unsigned)(i * 47 + 9);
        acc = rotl32(acc, 9u);
        seed = out * 0.46875L + seed * 0.5L + (long double)i * 0.0078125L;
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
