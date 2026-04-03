#include <stdio.h>

typedef struct {
    long double x;
    long double y;
    int z;
} W16Pack;

static long long quant16(long double x) {
    if (x >= 0.0L) return (long long)(x * 16.0L + 0.5L);
    return (long long)(x * 16.0L - 0.5L);
}

static W16Pack w16_make(int seed) {
    W16Pack v;
    v.x = (long double)seed + 0.5L;
    v.y = (long double)seed * 2.0L;
    v.z = seed + 3;
    return v;
}

static long long w16_score(W16Pack v) {
    long double mix = v.x + v.y * 0.5L + (long double)v.z * 0.25L;
    return quant16(mix);
}

int main(void) {
    W16Pack value = w16_make(4);
    printf("%lld\n", w16_score(value));
    return 0;
}
