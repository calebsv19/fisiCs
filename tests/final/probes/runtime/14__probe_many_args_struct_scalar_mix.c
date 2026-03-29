#include <stdio.h>

typedef struct Pair {
    int a;
    int b;
} Pair;

static long long mix(
    Pair p0, int x0, long long y0,
    Pair p1, unsigned u0, int x1,
    Pair p2, long long y1, unsigned u1, int x2
) {
    long long acc = 0;
    acc += (long long)p0.a + (long long)p0.b * 2;
    acc += (long long)x0 * 3 + y0;
    acc += (long long)p1.a * 5 + (long long)p1.b * 7;
    acc += (long long)u0 + (long long)x1 * 11;
    acc += (long long)p2.a * 13 + (long long)p2.b * 17;
    acc += y1 * 19 + (long long)u1 * 23 + (long long)x2 * 29;
    return acc;
}

int main(void) {
    Pair p0 = {1, 2};
    Pair p1 = {3, 4};
    Pair p2 = {5, 6};

    long long a = mix(p0, 7, 11, p1, 13u, 17, p2, 19, 23u, 29);
    long long b = mix(p2, -3, 5, p0, 9u, -7, p1, 2, 15u, -11);

    printf("%lld %lld\n", a, b);
    return 0;
}
