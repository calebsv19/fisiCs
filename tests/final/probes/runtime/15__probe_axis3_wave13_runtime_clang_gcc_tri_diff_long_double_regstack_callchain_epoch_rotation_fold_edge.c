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

static long double axis3_w13_leaf(
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
        a0 + a1 * 0.453125L + a2 * 0.3125L + a3 * 0.171875L + a4 * 0.0859375L + a5 * 0.04296875L;
    sum += (long double)(u0 + u2 + u4) * 0.0126953125L;
    sum += (long double)(u1 + u3 + u5) * 0.009765625L;
    sum += (long double)(s0 - s1 + s2 - s3 + s4) * 0.0234375L;
    return sum;
}

static long double axis3_w13_stage(
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
        seed + (long double)(rotl32((epoch + frontier) ^ replay, (frontier ^ replay) & 31u) & 31u)
                   * 0.0126953125L;
    return axis3_w13_leaf(
        base,
        base * 0.6953125L + (long double)round * 0.1015625L,
        base * 0.5078125L + (long double)(p0 & 31u) * 0.05078125L,
        base * 0.2890625L + (long double)(p1 & 31u) * 0.025390625L,
        base * 0.14453125L + (long double)(p2 & 15u) * 0.0126953125L,
        base * 0.07421875L + (long double)(p3 & 15u) * 0.00634765625L,
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
    unsigned epoch = 149u;
    unsigned frontier = 17u;
    unsigned replay = 5u;
    long double seed = 1.4375L;
    int i;

    for (i = 0; i < 15; ++i) {
        long double x0 = axis3_w13_stage(
            seed,
            epoch,
            frontier,
            replay,
            i,
            (unsigned)(53 + i * 13),
            (unsigned)(67 + i * 17),
            (unsigned)(79 + i * 19),
            (unsigned)(101 + i * 23),
            (unsigned)(127 + i * 29),
            (unsigned)(149 + i * 31)
        );
        long double x1 = axis3_w13_stage(
            seed + x0 * 0.0859375L,
            epoch ^ (unsigned)(i * 43 + 13),
            frontier ^ (unsigned)(i * 31 + 9),
            replay ^ (unsigned)(i * 29 + 5),
            i + 9,
            (unsigned)(163 + i * 17),
            (unsigned)(191 + i * 11),
            (unsigned)(223 + i * 13),
            (unsigned)(251 + i * 17),
            (unsigned)(277 + i * 19),
            (unsigned)(307 + i * 23)
        );
        long double x2 = axis3_w13_stage(
            seed + x1 * 0.06640625L,
            epoch ^ (unsigned)(i * 47 + 7),
            frontier ^ (unsigned)(i * 23 + 11),
            replay ^ (unsigned)(i * 37 + 3),
            i + 11,
            (unsigned)(331 + i * 11),
            (unsigned)(349 + i * 13),
            (unsigned)(373 + i * 7),
            (unsigned)(397 + i * 9),
            (unsigned)(421 + i * 11),
            (unsigned)(449 + i * 13)
        );
        long double out = x0 * 0.453125L + x1 * 0.3515625L + x2 * 0.1953125L
                          + (long double)(frontier & 31u) * 0.0048828125L;

        acc ^= qld(out) + (unsigned)(i * 89 + 31);
        acc = rotl32(acc, 11u);
        seed = out * 0.4609375L + seed * 0.4296875L + (long double)i * 0.0048828125L;
        epoch = rotl32(epoch ^ (unsigned)(i * 59 + 19), (unsigned)((i & 7) + 5));
        frontier = rotl32(frontier ^ (unsigned)(i * 41 + 11), (unsigned)((i & 7) + 6));
        replay = rotl32(replay ^ (unsigned)(i * 31 + 5), (unsigned)((i & 7) + 4));
    }

    printf("%u %u\n", acc, qld(seed));
    return 0;
}
