#include <stdio.h>

struct Wave83Tuple {
    long double total;
    int count;
};

struct Wave83Tuple wave83_collect(long double seed, int count, ...);
long double wave83_mix(struct Wave83Tuple t, long double scale);

static long long quant16(long double x) {
    if (x >= 0.0L) {
        return (long long)(x * 16.0L + 0.5L);
    }
    return (long long)(x * 16.0L - 0.5L);
}

int main(void) {
    struct Wave83Tuple t = wave83_collect(1.0L, 4, 0.5L, -1.25L, 2.75L, 1.5L);
    long double mixed = wave83_mix(t, 0.625L);
    printf("%lld %d\n", quant16(mixed), t.count);
    return 0;
}
