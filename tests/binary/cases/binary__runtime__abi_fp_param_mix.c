#include <stdio.h>

static long long fold_fp_mix(
    int a,
    double b,
    int c,
    double d,
    int e,
    double f) {
    double total = (double)a + b + (double)c + d + (double)e + f;
    return (long long)(total * 4.0);
}

int main(void) {
    long long value = fold_fp_mix(1, 1.5, 2, 2.25, 3, 3.75);
    printf("%lld\n", value);
    return 0;
}
