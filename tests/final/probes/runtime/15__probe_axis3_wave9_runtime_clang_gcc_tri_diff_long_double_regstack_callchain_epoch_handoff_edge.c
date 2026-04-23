#include <stdio.h>

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned qld(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.8125L) + 0.5L;
    }
    return (unsigned)(v * 1024.0L + 0.5L);
}

static long double axis3_w9_leaf(
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
    long double sum =
        a0 + a1 * 0.5L + a2 * 0.296875L + a3 * 0.1484375L + a4 * 0.0703125L + a5 * 0.03515625L;
    sum += (long double)(u0 + u2 + u4) * 0.015625L;
    sum += (long double)(u1 + u3 + u5) * 0.0087890625L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4) * 0.03125L;
    return sum;
}

static long double axis3_w9_stage(
    long double seed,
    unsigned epoch,
    unsigned frontier,
    unsigned replay,
    int round,
    unsigned p0,
    unsigned p1,
    unsigned p2,
    unsigned p3,
    unsigned p4,
    unsigned p5
) {
    long double base =
        seed + (long double)(rotl32(epoch ^ replay, frontier & 31u) & 31u) * 0.015625L;
    return axis3_w9_leaf(
        base,
        base * 0.75L + (long double)round * 0.1015625L,
        base * 0.546875L + (long double)(p0 & 31u) * 0.05078125L,
        base * 0.2734375L + (long double)(p1 & 31u) * 0.025390625L,
        base * 0.13671875L + (long double)(p2 & 15u) * 0.0126953125L,
        base * 0.0625L + (long double)(p3 & 15u) * 0.00634765625L,
        p0, p1, p2, p3, p4, p5,
        round,
        (int)((p2 ^ replay) & 63u),
        (int)((p3 ^ frontier) & 31u),
        (int)(p4 & 15u),
        (int)((replay >> 2) & 15u)
    );
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    unsigned epoch = 73u;
    unsigned frontier = 23u;
    unsigned replay = 11u;
    long double seed = 1.8125L;
    int i;

    for (i = 0; i < 12; ++i) {
        long double x0 = axis3_w9_stage(
            seed,
            epoch,
            frontier,
            replay,
            i,
            (unsigned)(41 + i * 7),
            (unsigned)(47 + i * 9),
            (unsigned)(61 + i * 11),
            (unsigned)(79 + i * 13),
            (unsigned)(97 + i * 17),
            (unsigned)(109 + i * 19)
        );
        long double x1 = axis3_w9_stage(
            seed + x0 * 0.078125L,
            epoch ^ (unsigned)(i * 31 + 9),
            frontier ^ (unsigned)(i * 19 + 5),
            replay ^ (unsigned)(i * 17 + 3),
            i + 6,
            (unsigned)(113 + i * 23),
            (unsigned)(131 + i * 29),
            (unsigned)(149 + i * 31),
            (unsigned)(167 + i * 7),
            (unsigned)(191 + i * 9),
            (unsigned)(223 + i * 11)
        );
        long double x2 = axis3_w9_stage(
            seed + x1 * 0.0546875L,
            epoch ^ (unsigned)(i * 41 + 1),
            frontier ^ (unsigned)(i * 13 + 7),
            replay ^ (unsigned)(i * 23 + 5),
            i + 8,
            (unsigned)(233 + i * 13),
            (unsigned)(251 + i * 5),
            (unsigned)(269 + i * 7),
            (unsigned)(281 + i * 3),
            (unsigned)(293 + i * 5),
            (unsigned)(311 + i * 7)
        );
        long double out = x0 * 0.5L + x1 * 0.3125L + x2 * 0.1875L
                          + (long double)(frontier & 31u) * 0.0078125L;

        acc ^= qld(out) + (unsigned)(i * 71 + 19);
        acc = rotl32(acc, 11u);
        seed = out * 0.5L + seed * 0.4375L + (long double)i * 0.0078125L;
        epoch = rotl32(epoch ^ (unsigned)(i * 43 + 13), (unsigned)((i & 7) + 5));
        frontier = rotl32(frontier ^ (unsigned)(i * 29 + 7), (unsigned)((i & 7) + 4));
        replay = rotl32(replay ^ (unsigned)(i * 19 + 3), (unsigned)((i & 7) + 6));
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
