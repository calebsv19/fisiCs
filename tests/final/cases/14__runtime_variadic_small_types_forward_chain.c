#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

static long long consume(int lanes, va_list ap) {
    long long acc = 0;
    for (int i = 0; i < lanes; ++i) {
        int kind = va_arg(ap, int);
        if (kind == 0) {
            int v = va_arg(ap, int);
            acc += (long long)v * 11LL;
        } else if (kind == 1) {
            int v = va_arg(ap, int);
            acc += (long long)v * 13LL;
        } else if (kind == 2) {
            int v = va_arg(ap, int);
            acc += (long long)v * 17LL;
        } else if (kind == 3) {
            double d = va_arg(ap, double);
            acc += (long long)(d * 100.0);
        } else {
            int v = va_arg(ap, int);
            acc += (long long)v;
        }
    }
    return acc;
}

static long long relay_twice(int lanes, ...) {
    va_list ap;
    va_list cp;
    va_start(ap, lanes);
    va_copy(cp, ap);
    long long a = consume(lanes, ap);
    long long b = consume(lanes, cp);
    va_end(cp);
    va_end(ap);
    return a * 10LL + b;
}

int main(void) {
    long long out = relay_twice(
        5,
        0, (signed char)-5,
        1, (unsigned char)240,
        2, (short)-12,
        3, (float)1.75f,
        4, 29
    );
    printf("%lld\n", out);
    return 0;
}
