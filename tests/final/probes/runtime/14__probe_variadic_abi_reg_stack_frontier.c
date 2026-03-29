#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

static long long fold_variadic_frontier(int base_i, long long base_k, double base_d, int lanes, ...) {
    va_list ap;
    long long acc = (long long)base_i * 17 + base_k * 3 + (long long)(base_d * 100.0);

    va_start(ap, lanes);
    for (int i = 0; i < lanes; ++i) {
        int kind = va_arg(ap, int);
        long long value = 0;
        if (kind == 0) {
            value = (long long)va_arg(ap, int);
        } else if (kind == 1) {
            value = va_arg(ap, long long);
        } else {
            value = (long long)(va_arg(ap, double) * 100.0);
        }
        acc = acc * 131 + value * (long long)(i + 5);
    }
    va_end(ap);

    return acc;
}

int main(void) {
    long long out = fold_variadic_frontier(
        9, 123LL, 1.25, 8,
        0, -3,
        1, 4000LL,
        2, 2.5,
        0, 7,
        2, -0.75,
        1, -81LL,
        0, 11,
        2, 3.0
    );

    printf("%lld\n", out);
    return 0;
}
