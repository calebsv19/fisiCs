#include <stdio.h>

static long long mix(int a, unsigned b, long long c, float d, double e, int f, unsigned g) {
    return (long long)a + (long long)b + c + (long long)(d * 10.0f) + (long long)(e * 10.0) +
           (long long)f + (long long)g;
}

int main(void) {
    long long out = mix(3, 4u, 10000000000LL, 1.5f, 2.25, -7, 9u);
    printf("%lld\n", out);
    return 0;
}
