#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

static long long consume(int count, va_list ap) {
    long long acc = 0;
    for (int i = 0; i < count; ++i) {
        int tag = va_arg(ap, int);
        if (tag == 0) {
            int v = va_arg(ap, int);
            acc += (long long)v * 5;
        } else if (tag == 1) {
            unsigned int v = va_arg(ap, unsigned int);
            acc += (long long)v * 3;
        } else if (tag == 2) {
            double d = va_arg(ap, double);
            acc += (long long)(d * 100.0);
        } else {
            int v = va_arg(ap, int);
            acc -= v;
        }
    }
    return acc;
}

static long long run_twice(int count, ...) {
    va_list ap;
    va_list cp;
    va_start(ap, count);
    va_copy(cp, ap);
    long long a = consume(count, ap);
    long long b = consume(count, cp);
    va_end(cp);
    va_end(ap);
    return a * 1000 + b;
}

int main(void) {
    signed char sc = -7;
    unsigned char uc = 240u;
    short sh = -120;
    unsigned short ush = 65000u;
    float f = 2.25f;

    long long out = run_twice(5, 0, sc, 1, uc, 3, sh, 1, ush, 2, f);
    printf("%lld\n", out);
    return 0;
}
