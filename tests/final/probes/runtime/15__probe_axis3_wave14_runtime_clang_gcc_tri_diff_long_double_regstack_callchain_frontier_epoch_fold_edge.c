#include <stdio.h>

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned qld(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.5625L) + 0.5L;
    }
    return (unsigned)(v * 1024.0L + 0.5L);
}

static long double axis3_w14_leaf(
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
        a0 + a1 * 0.44140625L + a2 * 0.32421875L + a3 * 0.16796875L + a4 * 0.08203125L + a5 * 0.046875L;
    sum += (long double)(u0 + u2 + u4) * 0.01171875L;
    sum += (long double)(u1 + u3 + u5) * 0.0107421875L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4) * 0.021484375L;
    return sum;
}

static long double axis3_w14_stage(
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
        seed + (long double)(rotl32((epoch ^ frontier) + replay, (frontier ^ replay) & 31u) & 31u)
                   * 0.01171875L;
    return axis3_w14_leaf(
        base,
        base * 0.68359375L + (long double)round * 0.09765625L,
        base * 0.51953125L + (long double)(p0 & 31u) * 0.046875L,
        base * 0.27734375L + (long double)(p1 & 31u) * 0.02734375L,
        base * 0.15234375L + (long double)(p2 & 15u) * 0.01171875L,
        base * 0.078125L + (long double)(p3 & 15u) * 0.005859375L,
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
    unsigned epoch = 173u;
    unsigned frontier = 19u;
    unsigned replay = 7u;
    long double seed = 1.34375L;
    int i;

    for (i = 0; i < 16; ++i) {
        long double x0 = axis3_w14_stage(
            seed,
            epoch,
            frontier,
            replay,
            i,
            (unsigned)(59 + i * 17),
            (unsigned)(71 + i * 19),
            (unsigned)(89 + i * 23),
            (unsigned)(109 + i * 29),
            (unsigned)(137 + i * 31),
            (unsigned)(163 + i * 37)
        );
        long double x1 = axis3_w14_stage(
            seed + x0 * 0.08203125L,
            epoch ^ (unsigned)(i * 47 + 13),
            frontier ^ (unsigned)(i * 37 + 9),
            replay ^ (unsigned)(i * 31 + 5),
            i + 9,
            (unsigned)(181 + i * 19),
            (unsigned)(211 + i * 13),
            (unsigned)(241 + i * 17),
            (unsigned)(271 + i * 19),
            (unsigned)(307 + i * 23),
            (unsigned)(337 + i * 29)
        );
        long double x2 = axis3_w14_stage(
            seed + x1 * 0.0625L,
            epoch ^ (unsigned)(i * 53 + 7),
            frontier ^ (unsigned)(i * 29 + 11),
            replay ^ (unsigned)(i * 41 + 3),
            i + 11,
            (unsigned)(359 + i * 13),
            (unsigned)(389 + i * 17),
            (unsigned)(419 + i * 11),
            (unsigned)(443 + i * 13),
            (unsigned)(467 + i * 17),
            (unsigned)(503 + i * 19)
        );
        long double out = x0 * 0.4453125L + x1 * 0.359375L + x2 * 0.1953125L
                          + (long double)(frontier & 31u) * 0.00439453125L;

        acc ^= qld(out) + (unsigned)(i * 97 + 37);
        acc = rotl32(acc, 11u);
        seed = out * 0.453125L + seed * 0.421875L + (long double)i * 0.00439453125L;
        epoch = rotl32(epoch ^ (unsigned)(i * 61 + 19), (unsigned)((i & 7) + 5));
        frontier = rotl32(frontier ^ (unsigned)(i * 43 + 11), (unsigned)((i & 7) + 6));
        replay = rotl32(replay ^ (unsigned)(i * 37 + 5), (unsigned)((i & 7) + 4));
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
