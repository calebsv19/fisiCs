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
            int v = va_arg(ap, int);
            acc += (long long)v;
        } else if (kind == 1) {
            double d = va_arg(ap, double);
            acc += (long long)(d * 100.0);
        } else if (kind == 2) {
            unsigned long long u = va_arg(ap, unsigned long long);
            acc += (long long)(u % 1000000ull);
        } else {
            long long ll = va_arg(ap, long long);
            acc += ll;
        }
    }

    va_end(ap);
    return acc;
}

int main(void) {
    long long total = gather(
        8,
        0, -7,
        1, 1.25,
        2, 9000000123456ull,
        3, -1234567ll,
        1, -2.5,
        2, 42ull,
        0, 19,
        3, 777ll
    );

    printf("%lld\n", total);
    return 0;
}
