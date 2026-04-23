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

static long double axis3_w15_leaf(
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
        a0 + a1 * 0.43359375L + a2 * 0.33203125L + a3 * 0.16015625L + a4 * 0.08984375L + a5 * 0.05078125L;
    sum += (long double)(u0 + u2 + u4) * 0.0107421875L;
    sum += (long double)(u1 + u3 + u5) * 0.01171875L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4) * 0.01953125L;
    return sum;
}

static long double axis3_w15_stage(
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
        seed + (long double)(rotl32((epoch ^ frontier) + replay, (frontier ^ replay ^ epoch) & 31u) & 31u)
                   * 0.0107421875L;
    return axis3_w15_leaf(
        base,
        base * 0.67578125L + (long double)round * 0.09375L,
        base * 0.53125L + (long double)(p0 & 31u) * 0.04296875L,
        base * 0.265625L + (long double)(p1 & 31u) * 0.029296875L,
        base * 0.16015625L + (long double)(p2 & 15u) * 0.0107421875L,
        base * 0.08203125L + (long double)(p3 & 15u) * 0.00537109375L,
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
    unsigned epoch = 197u;
    unsigned frontier = 23u;
    unsigned replay = 11u;
    long double seed = 1.28125L;
    int i;

    for (i = 0; i < 18; ++i) {
        long double x0 = axis3_w15_stage(
            seed,
            epoch,
            frontier,
            replay,
            i,
            (unsigned)(67 + i * 19),
            (unsigned)(79 + i * 23),
            (unsigned)(97 + i * 29),
            (unsigned)(113 + i * 31),
            (unsigned)(149 + i * 37),
            (unsigned)(181 + i * 41)
        );
        long double x1 = axis3_w15_stage(
            seed + x0 * 0.078125L,
            epoch ^ (unsigned)(i * 53 + 13),
            frontier ^ (unsigned)(i * 41 + 9),
            replay ^ (unsigned)(i * 37 + 5),
            i + 9,
            (unsigned)(199 + i * 23),
            (unsigned)(229 + i * 17),
            (unsigned)(263 + i * 19),
            (unsigned)(293 + i * 23),
            (unsigned)(337 + i * 29),
            (unsigned)(373 + i * 31)
        );
        long double x2 = axis3_w15_stage(
            seed + x1 * 0.05859375L,
            epoch ^ (unsigned)(i * 59 + 7),
            frontier ^ (unsigned)(i * 31 + 11),
            replay ^ (unsigned)(i * 43 + 3),
            i + 11,
            (unsigned)(397 + i * 17),
            (unsigned)(431 + i * 19),
            (unsigned)(463 + i * 13),
            (unsigned)(491 + i * 17),
            (unsigned)(523 + i * 19),
            (unsigned)(557 + i * 23)
        );
        long double x3 = axis3_w15_stage(
            seed + x2 * 0.041015625L,
            epoch ^ (unsigned)(i * 61 + 5),
            frontier ^ (unsigned)(i * 37 + 13),
            replay ^ (unsigned)(i * 47 + 1),
            i + 13,
            (unsigned)(587 + i * 11),
            (unsigned)(613 + i * 13),
            (unsigned)(641 + i * 17),
            (unsigned)(673 + i * 19),
            (unsigned)(701 + i * 23),
            (unsigned)(733 + i * 29)
        );
        long double out = x0 * 0.41796875L + x1 * 0.31640625L + x2 * 0.19140625L + x3 * 0.07421875L
                          + (long double)(frontier & 63u) * 0.00390625L;

        acc ^= qld(out) + (unsigned)(i * 101 + 41);
        acc = rotl32(acc, 11u);
        seed = out * 0.44140625L + seed * 0.41015625L + (long double)i * 0.00390625L;
        epoch = rotl32(epoch ^ (unsigned)(i * 67 + 19), (unsigned)((i & 7) + 5));
        frontier = rotl32(frontier ^ (unsigned)(i * 47 + 11), (unsigned)((i & 7) + 6));
        replay = rotl32(replay ^ (unsigned)(i * 41 + 5), (unsigned)((i & 7) + 4));
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
