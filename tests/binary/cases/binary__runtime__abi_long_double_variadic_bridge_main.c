#include <stdio.h>

struct W16Tuple {
    long double total;
    int count;
};

struct W16Tuple w16_collect(long double seed, int count, ...);
long double w16_mix(struct W16Tuple t, long double scale);

static long long quant16(long double x) {
    if (x >= 0.0L) return (long long)(x * 16.0L + 0.5L);
    return (long long)(x * 16.0L - 0.5L);
}

int main(void) {
    struct W16Tuple t = w16_collect(1.0L, 4, 0.5L, -1.25L, 2.75L, 1.5L);
    long double mixed = w16_mix(t, 0.625L);
    printf("%lld %d\n", quant16(mixed), t.count);
    return 0;
}
