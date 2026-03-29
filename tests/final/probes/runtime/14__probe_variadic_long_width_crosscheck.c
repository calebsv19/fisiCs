#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

static long long gather(int lanes, ...) {
    va_list ap;
    va_start(ap, lanes);

    long long acc = 0;
    for (int i = 0; i < lanes; ++i) {
        int kind = va_arg(ap, int);
        if (kind == 0) {
            long v = va_arg(ap, long);
            acc += (long long)v;
        } else if (kind == 1) {
            unsigned long u = va_arg(ap, unsigned long);
            acc += (long long)(u % 1000000ul);
        } else if (kind == 2) {
            long long ll = va_arg(ap, long long);
            acc += ll;
        } else {
            double d = va_arg(ap, double);
            acc += (long long)(d * 10.0);
        }
    }

    va_end(ap);
    return acc;
}

int main(void) {
    long long out = gather(
        8,
        0, (long)-7,
        1, (unsigned long)123456789ul,
        2, (long long)-4000,
        3, 2.5,
        0, (long)42,
        1, (unsigned long)9000005ul,
        2, (long long)700,
        3, -1.5
    );

    printf("%lld\n", out);
    return 0;
}
