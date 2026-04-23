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

static long double axis3_w12_leaf(
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
        a0 + a1 * 0.46875L + a2 * 0.328125L + a3 * 0.1640625L + a4 * 0.08203125L + a5 * 0.0390625L;
    sum += (long double)(u0 + u2 + u4) * 0.013671875L;
    sum += (long double)(u1 + u3 + u5) * 0.0107421875L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4) * 0.02734375L;
    return sum;
}

static long double axis3_w12_stage(
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
        seed + (long double)(rotl32(epoch ^ replay, frontier & 31u) & 31u) * 0.013671875L;
    return axis3_w12_leaf(
        base,
        base * 0.71875L + (long double)round * 0.1171875L,
        base * 0.515625L + (long double)(p0 & 31u) * 0.05859375L,
        base * 0.296875L + (long double)(p1 & 31u) * 0.029296875L,
        base * 0.1484375L + (long double)(p2 & 15u) * 0.0146484375L,
        base * 0.078125L + (long double)(p3 & 15u) * 0.00732421875L,
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
    unsigned epoch = 109u;
    unsigned frontier = 13u;
    unsigned replay = 3u;
    long double seed = 1.5625L;
    int i;

    for (i = 0; i < 14; ++i) {
        long double x0 = axis3_w12_stage(
            seed,
            epoch,
            frontier,
            replay,
            i,
            (unsigned)(47 + i * 11),
            (unsigned)(59 + i * 13),
            (unsigned)(73 + i * 17),
            (unsigned)(97 + i * 19),
            (unsigned)(113 + i * 23),
            (unsigned)(139 + i * 29)
        );
        long double x1 = axis3_w12_stage(
            seed + x0 * 0.09375L,
            epoch ^ (unsigned)(i * 41 + 13),
            frontier ^ (unsigned)(i * 29 + 9),
            replay ^ (unsigned)(i * 23 + 5),
            i + 8,
            (unsigned)(151 + i * 31),
            (unsigned)(173 + i * 7),
            (unsigned)(199 + i * 11),
            (unsigned)(229 + i * 13),
            (unsigned)(257 + i * 17),
            (unsigned)(281 + i * 19)
        );
        long double x2 = axis3_w12_stage(
            seed + x1 * 0.0703125L,
            epoch ^ (unsigned)(i * 47 + 5),
            frontier ^ (unsigned)(i * 19 + 11),
            replay ^ (unsigned)(i * 31 + 7),
            i + 10,
            (unsigned)(293 + i * 13),
            (unsigned)(307 + i * 7),
            (unsigned)(331 + i * 9),
            (unsigned)(359 + i * 5),
            (unsigned)(383 + i * 7),
            (unsigned)(409 + i * 9)
        );
        long double out = x0 * 0.46875L + x1 * 0.34375L + x2 * 0.1875L
                          + (long double)(frontier & 31u) * 0.005859375L;

        acc ^= qld(out) + (unsigned)(i * 83 + 29);
        acc = rotl32(acc, 11u);
        seed = out * 0.4765625L + seed * 0.4453125L + (long double)i * 0.005859375L;
        epoch = rotl32(epoch ^ (unsigned)(i * 53 + 19), (unsigned)((i & 7) + 5));
        frontier = rotl32(frontier ^ (unsigned)(i * 37 + 11), (unsigned)((i & 7) + 6));
        replay = rotl32(replay ^ (unsigned)(i * 29 + 5), (unsigned)((i & 7) + 4));
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
