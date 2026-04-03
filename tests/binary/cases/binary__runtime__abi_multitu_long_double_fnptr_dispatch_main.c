#include <stdio.h>

typedef long double (*w17_op)(long double, long double);

w17_op w17_pick(int id);
long double w17_apply(w17_op fn, long double a, long double b);

static long long quant16(long double x) {
    if (x >= 0.0L) return (long long)(x * 16.0L + 0.5L);
    return (long long)(x * 16.0L - 0.5L);
}

int main(void) {
    long double r1 = w17_apply(w17_pick(0), 1.5L, 2.0L);
    long double r2 = w17_apply(w17_pick(1), 2.0L, 1.25L);
    printf("%lld\n", quant16(r1 + r2));
    return 0;
}
