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
            acc += (long long)(d * 10.0);
        } else if (kind == 2) {
            unsigned int u = va_arg(ap, unsigned int);
            acc += (long long)u;
        } else {
            long long ll = va_arg(ap, long long);
            acc += ll;
        }
    }

    va_end(ap);
    return acc;
}

int main(void) {
    signed char sc = -3;
    unsigned char uc = 250;
    float f = 1.25f;
    short sh = -7;

    long long total = gather(
        8,
        0, sc,
        0, uc,
        1, f,
        1, -2.5,
        2, 4000000000u,
        3, 1234567890123LL,
        0, sh,
        2, 42u
    );

    printf("%lld\n", total);
    return 0;
}
