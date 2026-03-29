#include <stdio.h>

static unsigned fold_bytes(const unsigned char *bytes, int n) {
    unsigned hash = 2166136261u;
    for (int i = 0; i < n; ++i) {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}

static long double ld_pick(int which, long double a, long double b) {
    return which ? a : b;
}

int main(void) {
    long double a = 1.0L;
    long double b = -0.0L;
    long double c = 3.5L;
    long double r1 = ld_pick(1, a, b);
    long double r2 = ld_pick(0, c, r1);
    unsigned h1 = fold_bytes((const unsigned char *)&r1, (int)sizeof(r1));
    unsigned h2 = fold_bytes((const unsigned char *)&r2, (int)sizeof(r2));
    printf("%zu %u %u\n", sizeof(long double), h1, h2);
    return 0;
}
