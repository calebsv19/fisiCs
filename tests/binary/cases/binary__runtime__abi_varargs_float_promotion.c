#include <stdarg.h>
#include <stdio.h>

static long long sum_scaled(int count, ...) {
    va_list ap;
    int i;
    double acc = 0.0;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        double v = va_arg(ap, double);
        acc += v;
    }
    va_end(ap);

    return (long long)(acc * 8.0);
}

int main(void) {
    float a = 1.5f;
    float b = 2.25f;
    double c = 3.5;
    printf("%lld\n", sum_scaled(3, a, b, c));
    return 0;
}
