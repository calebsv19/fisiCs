#include <stdio.h>

static unsigned rotl32(unsigned x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static unsigned qld(long double v) {
    if (v < 0.0L) {
        v = (-v * 1.6875L) + 0.5L;
    }
    return (unsigned)(v * 1024.0L + 0.5L);
}

static long double axis3_w10_leaf(
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
        a0 + a1 * 0.484375L + a2 * 0.3125L + a3 * 0.15625L + a4 * 0.078125L + a5 * 0.03125L;
    sum += (long double)(u0 + u2 + u4) * 0.0146484375L;
    sum += (long double)(u1 + u3 + u5) * 0.009765625L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4) * 0.029296875L;
    return sum;
}

static long double axis3_w10_stage(
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
        seed + (long double)(rotl32(epoch ^ replay, frontier & 31u) & 31u) * 0.0146484375L;
    return axis3_w10_leaf(
        base,
        base * 0.734375L + (long double)round * 0.109375L,
        base * 0.53125L + (long double)(p0 & 31u) * 0.0546875L,
        base * 0.28125L + (long double)(p1 & 31u) * 0.02734375L,
        base * 0.140625L + (long double)(p2 & 15u) * 0.013671875L,
        base * 0.0703125L + (long double)(p3 & 15u) * 0.0068359375L,
        p0, p1, p2, p3, p4, p5,
        round,
        (int)((p2 ^ replay) & 63u),
        (int)((p3 ^ frontier) & 31u),
        (int)(p4 & 15u),
        (int)((replay >> 1) & 15u)
    );
}

int main(void) {
    unsigned acc = 0x811c9dc5u;
    unsigned epoch = 89u;
    unsigned frontier = 17u;
    unsigned replay = 5u;
    long double seed = 1.6875L;
    int i;

    for (i = 0; i < 13; ++i) {
        long double x0 = axis3_w10_stage(
            seed,
            epoch,
            frontier,
            replay,
            i,
            (unsigned)(43 + i * 9),
            (unsigned)(53 + i * 11),
            (unsigned)(67 + i * 13),
            (unsigned)(83 + i * 17),
            (unsigned)(101 + i * 19),
            (unsigned)(127 + i * 23)
        );
        long double x1 = axis3_w10_stage(
            seed + x0 * 0.0859375L,
            epoch ^ (unsigned)(i * 37 + 11),
            frontier ^ (unsigned)(i * 23 + 7),
            replay ^ (unsigned)(i * 19 + 3),
            i + 7,
            (unsigned)(137 + i * 29),
            (unsigned)(149 + i * 31),
            (unsigned)(173 + i * 7),
            (unsigned)(197 + i * 9),
            (unsigned)(227 + i * 11),
            (unsigned)(251 + i * 13)
        );
        long double x2 = axis3_w10_stage(
            seed + x1 * 0.0625L,
            epoch ^ (unsigned)(i * 43 + 3),
            frontier ^ (unsigned)(i * 17 + 9),
            replay ^ (unsigned)(i * 29 + 5),
            i + 9,
            (unsigned)(257 + i * 13),
            (unsigned)(269 + i * 5),
            (unsigned)(283 + i * 7),
            (unsigned)(307 + i * 3),
            (unsigned)(331 + i * 5),
            (unsigned)(353 + i * 7)
        );
        long double out = x0 * 0.484375L + x1 * 0.328125L + x2 * 0.1875L
                          + (long double)(frontier & 31u) * 0.0068359375L;

        acc ^= qld(out) + (unsigned)(i * 79 + 23);
        acc = rotl32(acc, 13u);
        seed = out * 0.4921875L + seed * 0.4296875L + (long double)i * 0.0068359375L;
        epoch = rotl32(epoch ^ (unsigned)(i * 47 + 17), (unsigned)((i & 7) + 6));
        frontier = rotl32(frontier ^ (unsigned)(i * 31 + 9), (unsigned)((i & 7) + 5));
        replay = rotl32(replay ^ (unsigned)(i * 23 + 3), (unsigned)((i & 7) + 4));
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
