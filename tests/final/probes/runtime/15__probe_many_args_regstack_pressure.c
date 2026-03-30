#include <stdio.h>

typedef struct Pair {
    int x;
    int y;
} Pair;

static unsigned long long rotl64(unsigned long long v, unsigned s) {
    s &= 63u;
    if (s == 0u) {
        return v;
    }
    return (v << s) | (v >> (64u - s));
}

static unsigned long long frontier(
    int a0, long long b0, double c0, Pair p0,
    int a1, long long b1, double c1, Pair p1,
    int a2, long long b2, double c2, Pair p2,
    int a3, long long b3, double c3, Pair p3,
    int a4, long long b4, double c4, Pair p4,
    int a5, long long b5, double c5, Pair p5
) {
    unsigned long long acc = 0ULL;

    acc += (unsigned long long)(a0 + p0.x * 3 - p0.y) + (unsigned long long)(b0 + (long long)(c0 * 10.0));
    acc += (unsigned long long)(a1 + p1.x * 5 - p1.y) + (unsigned long long)(b1 + (long long)(c1 * 10.0));
    acc += (unsigned long long)(a2 + p2.x * 7 - p2.y) + (unsigned long long)(b2 + (long long)(c2 * 10.0));
    acc += (unsigned long long)(a3 + p3.x * 11 - p3.y) + (unsigned long long)(b3 + (long long)(c3 * 10.0));
    acc += (unsigned long long)(a4 + p4.x * 13 - p4.y) + (unsigned long long)(b4 + (long long)(c4 * 10.0));
    acc += (unsigned long long)(a5 + p5.x * 17 - p5.y) + (unsigned long long)(b5 + (long long)(c5 * 10.0));

    acc ^= (unsigned long long)(p0.x + p1.x + p2.x + p3.x + p4.x + p5.x) * 131ULL;
    acc ^= (unsigned long long)(p0.y + p1.y + p2.y + p3.y + p4.y + p5.y) * 257ULL;
    return acc;
}

int main(void) {
    Pair base[6] = {
        {2, 5}, {7, 3}, {11, 13}, {17, 19}, {23, 29}, {31, 37}
    };
    unsigned long long total = 0ULL;

    for (int i = 0; i < 40; ++i) {
        Pair p0 = {base[0].x + (i % 3), base[0].y + (i % 5)};
        Pair p1 = {base[1].x + (i % 4), base[1].y + (i % 3)};
        Pair p2 = {base[2].x + (i % 5), base[2].y + (i % 4)};
        Pair p3 = {base[3].x + (i % 6), base[3].y + (i % 5)};
        Pair p4 = {base[4].x + (i % 7), base[4].y + (i % 6)};
        Pair p5 = {base[5].x + (i % 8), base[5].y + (i % 7)};

        unsigned long long out = frontier(
            -3 + i, 1000LL + i * 31LL, 0.5 + (double)(i % 7) * 0.25, p0,
            5 + i * 2, 2000LL + i * 29LL, 1.25 + (double)(i % 5) * 0.5, p1,
            7 + i * 3, 3000LL + i * 23LL, 2.5 + (double)(i % 4) * 0.75, p2,
            11 + i * 4, 4000LL + i * 19LL, 3.75 + (double)(i % 3) * 1.0, p3,
            13 + i * 5, 5000LL + i * 17LL, 4.5 + (double)(i % 6) * 0.2, p4,
            17 + i * 6, 6000LL + i * 13LL, 5.25 + (double)(i % 8) * 0.125, p5
        );

        total ^= rotl64(out, (unsigned)i & 15u);
        total += out + (unsigned long long)(i * 97u);
    }

    printf("%llu\n", total);
    return 0;
}
