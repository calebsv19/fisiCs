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

static long double axis3_w3_leaf(
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
    int s0,
    int s1,
    int s2,
    int s3,
    int s4
) {
    long double sum = a0 + a1 * 0.5L + a2 * 0.25L + a3 * 0.125L + a4 * 0.0625L + a5 * 0.03125L;
    sum += (long double)(u0 + u2 + u4) * 0.015625L;
    sum += (long double)(u1 + u3 + u5) * 0.0078125L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4) * 0.03125L;
    return sum;
}

static long double axis3_w3_stage(long double seed, int round, unsigned p0, unsigned p1, unsigned p2, unsigned p3, unsigned p4, unsigned p5) {
    return axis3_w3_leaf(
        seed,
        seed * 0.75L + (long double)round * 0.125L,
        seed * 0.5L + (long double)(p0 & 31u) * 0.0625L,
        seed * 0.25L + (long double)(p1 & 15u) * 0.03125L,
        seed * 0.125L + (long double)(p2 & 15u) * 0.015625L,
        seed * 0.0625L + (long double)(p3 & 7u) * 0.0078125L,
        p0, p1, p2, p3, p4, p5,
        round, (int)(p2 & 31u), (int)(p3 & 15u), (int)(p4 & 15u), (int)(p5 & 7u)
    );
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    long double seed = 1.25L;
    int i;

    for (i = 0; i < 12; ++i) {
        long double x0 = axis3_w3_stage(seed, i, (unsigned)(17 + i * 3), (unsigned)(23 + i * 5), (unsigned)(31 + i * 7), (unsigned)(43 + i * 11), (unsigned)(59 + i * 13), (unsigned)(71 + i * 17));
        long double x1 = axis3_w3_stage(seed + x0 * 0.0625L, i + 3, (unsigned)(79 + i * 19), (unsigned)(97 + i * 23), (unsigned)(109 + i * 29), (unsigned)(127 + i * 31), (unsigned)(149 + i * 5), (unsigned)(173 + i * 7));
        long double out = x0 * 0.625L + x1 * 0.375L + (long double)i * 0.03125L;

        acc ^= qld(out) + (unsigned)(i * 41 + 9);
        acc = rotl32(acc, 7u);
        seed = out * 0.5L + seed * 0.4375L + (long double)i * 0.015625L;
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
