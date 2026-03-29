#include <stdio.h>

struct Pair {
    long long a;
    int b;
};

static long long mixcall(
    int a0,
    double d0,
    long long ll0,
    unsigned u0,
    struct Pair p0,
    int a1,
    double d1,
    long long ll1,
    unsigned u1,
    struct Pair p1
) {
    long long out = 0;
    out += (long long)a0 * 3LL + (long long)(d0 * 10.0) + ll0 - (long long)u0;
    out += p0.a + (long long)p0.b * 7LL;
    out += (long long)a1 * 5LL + (long long)(d1 * 10.0) + ll1 - (long long)u1;
    out += p1.a + (long long)p1.b * 11LL;
    return out;
}

int main(void) {
    struct Pair p0;
    struct Pair p1;
    p0.a = 901LL;
    p0.b = 4;
    p1.a = -120LL;
    p1.b = 9;
    long long v = mixcall(-3, 2.5, 700LL, 11u, p0, 8, 1.75, -90LL, 5u, p1);
    printf("%lld\n", v);
    return 0;
}
