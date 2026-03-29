#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

static long long fold_kinds(int lanes, ...) {
    va_list ap;
    va_start(ap, lanes);

    long long acc = 17;
    for (int i = 0; i < lanes; ++i) {
        int kind = va_arg(ap, int);
        long long v = 0;
        if (kind == 0) {
            v = (long long)va_arg(ap, int);
        } else if (kind == 1) {
            v = (long long)va_arg(ap, unsigned);
        } else if (kind == 2) {
            v = (long long)(va_arg(ap, double) * 100.0);
        } else {
            v = va_arg(ap, long long);
        }
        acc = acc * 97 + (long long)(i + 3) * v;
    }

    va_end(ap);
    return acc;
}

int main(void) {
    long long v = fold_kinds(
        7,
        0, -8,
        1, 420u,
        2, 1.25,
        3, 9000000001LL,
        0, 11,
        2, -0.5,
        1, 77u
    );
    printf("%lld\n", v);
    return 0;
}
