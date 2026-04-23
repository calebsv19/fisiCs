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

static long double axis3_w16_leaf(
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
        a0 + a1 * 0.42578125L + a2 * 0.33984375L + a3 * 0.15234375L + a4 * 0.09375L + a5 * 0.0546875L;
    sum += (long double)(u0 + u2 + u4) * 0.009765625L;
    sum += (long double)(u1 + u3 + u5) * 0.0126953125L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4) * 0.017578125L;
    return sum;
}

static long double axis3_w16_stage(
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
        seed + (long double)(rotl32((epoch ^ frontier) + replay, (frontier + replay + epoch) & 31u) & 31u)
                   * 0.009765625L;
    return axis3_w16_leaf(
        base,
        base * 0.66796875L + (long double)round * 0.08984375L,
        base * 0.54296875L + (long double)(p0 & 31u) * 0.0390625L,
        base * 0.25390625L + (long double)(p1 & 31u) * 0.03125L,
        base * 0.16796875L + (long double)(p2 & 15u) * 0.009765625L,
        base * 0.0859375L + (long double)(p3 & 15u) * 0.0048828125L,
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
    unsigned epoch = 239u;
    unsigned frontier = 29u;
    unsigned replay = 13u;
    long double seed = 1.21875L;
    int i;

    for (i = 0; i < 19; ++i) {
        long double x0 = axis3_w16_stage(
            seed,
            epoch,
            frontier,
            replay,
            i,
            (unsigned)(73 + i * 23),
            (unsigned)(89 + i * 29),
            (unsigned)(109 + i * 31),
            (unsigned)(127 + i * 37),
            (unsigned)(163 + i * 41),
            (unsigned)(197 + i * 43)
        );
        long double x1 = axis3_w16_stage(
            seed + x0 * 0.07421875L,
            epoch ^ (unsigned)(i * 59 + 13),
            frontier ^ (unsigned)(i * 47 + 9),
            replay ^ (unsigned)(i * 41 + 5),
            i + 9,
            (unsigned)(211 + i * 29),
            (unsigned)(241 + i * 19),
            (unsigned)(277 + i * 23),
            (unsigned)(311 + i * 29),
            (unsigned)(353 + i * 31),
            (unsigned)(389 + i * 37)
        );
        long double x2 = axis3_w16_stage(
            seed + x1 * 0.0546875L,
            epoch ^ (unsigned)(i * 61 + 7),
            frontier ^ (unsigned)(i * 37 + 11),
            replay ^ (unsigned)(i * 47 + 3),
            i + 11,
            (unsigned)(419 + i * 19),
            (unsigned)(457 + i * 23),
            (unsigned)(491 + i * 17),
            (unsigned)(523 + i * 19),
            (unsigned)(557 + i * 23),
            (unsigned)(593 + i * 29)
        );
        long double x3 = axis3_w16_stage(
            seed + x2 * 0.037109375L,
            epoch ^ (unsigned)(i * 67 + 5),
            frontier ^ (unsigned)(i * 43 + 13),
            replay ^ (unsigned)(i * 53 + 1),
            i + 13,
            (unsigned)(619 + i * 13),
            (unsigned)(647 + i * 17),
            (unsigned)(683 + i * 19),
            (unsigned)(719 + i * 23),
            (unsigned)(751 + i * 29),
            (unsigned)(787 + i * 31)
        );
        long double out = x0 * 0.3984375L + x1 * 0.3203125L + x2 * 0.203125L + x3 * 0.078125L
                          + (long double)(frontier & 63u) * 0.00341796875L;

        acc ^= qld(out) + (unsigned)(i * 109 + 43);
        acc = rotl32(acc, 11u);
        seed = out * 0.4296875L + seed * 0.3984375L + (long double)i * 0.00341796875L;
        epoch = rotl32(epoch ^ (unsigned)(i * 71 + 19), (unsigned)((i & 7) + 5));
        frontier = rotl32(frontier ^ (unsigned)(i * 53 + 11), (unsigned)((i & 7) + 6));
        replay = rotl32(replay ^ (unsigned)(i * 47 + 5), (unsigned)((i & 7) + 4));
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
