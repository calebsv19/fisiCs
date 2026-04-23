#include <stdio.h>

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned qld(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.625L) + 0.5L;
    }
    return (unsigned)(v * 1024.0L + 0.5L);
}

static long double axis3_w6_leaf(
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

static long double axis3_w6_stage(
    long double seed,
    unsigned epoch,
    unsigned frontier,
    int round,
    unsigned p0,
    unsigned p1,
    unsigned p2,
    unsigned p3,
    unsigned p4,
    unsigned p5
) {
    long double base = seed + (long double)(rotl32(epoch, frontier & 31u) & 31u) * 0.015625L;
    return axis3_w6_leaf(
        base,
        base * 0.75L + (long double)round * 0.125L,
        base * 0.5L + (long double)(p0 & 31u) * 0.0625L,
        base * 0.25L + (long double)(p1 & 31u) * 0.03125L,
        base * 0.125L + (long double)(p2 & 15u) * 0.015625L,
        base * 0.0625L + (long double)(p3 & 15u) * 0.0078125L,
        p0, p1, p2, p3, p4, p5,
        round,
        (int)(p2 & 63u),
        (int)(p3 & 31u),
        (int)(p4 & 15u),
        (int)((frontier >> 2) & 15u)
    );
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    unsigned epoch = 43u;
    unsigned frontier = 19u;
    long double seed = 1.625L;
    int i;

    for (i = 0; i < 10; ++i) {
        long double x0 = axis3_w6_stage(seed, epoch, frontier, i, (unsigned)(29 + i * 3), (unsigned)(37 + i * 5), (unsigned)(43 + i * 7), (unsigned)(59 + i * 11), (unsigned)(71 + i * 13), (unsigned)(89 + i * 17));
        long double x1 = axis3_w6_stage(seed + x0 * 0.0625L, epoch ^ (unsigned)(i * 31 + 7), frontier ^ (unsigned)(i * 13 + 5), i + 4, (unsigned)(97 + i * 19), (unsigned)(109 + i * 23), (unsigned)(127 + i * 29), (unsigned)(149 + i * 31), (unsigned)(173 + i * 5), (unsigned)(197 + i * 7));
        long double out = x0 * 0.5625L + x1 * 0.4375L + (long double)(frontier & 31u) * 0.0078125L;

        acc ^= qld(out) + (unsigned)(i * 47 + 11);
        acc = rotl32(acc, 11u);
        seed = out * 0.5L + seed * 0.46875L + (long double)i * 0.0078125L;
        epoch = rotl32(epoch ^ (unsigned)(i * 37 + 9), (unsigned)((i & 7) + 5));
        frontier = rotl32(frontier ^ (unsigned)(i * 19 + 3), (unsigned)((i & 7) + 4));
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
