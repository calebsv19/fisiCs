#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

static long long consume_pairs(int pairs, va_list ap) {
    long long acc = 0;
    for (int i = 0; i < pairs; ++i) {
        int tag = va_arg(ap, int);
        if (tag == 0) {
            acc += (long long)va_arg(ap, int);
        } else if (tag == 1) {
            acc += (long long)va_arg(ap, unsigned int);
        } else if (tag == 2) {
            acc += va_arg(ap, long long);
        } else if (tag == 3) {
            double d = va_arg(ap, double);
            acc += (long long)(d * 100.0);
        } else {
            double d = va_arg(ap, double);
            acc += (long long)(d * 10.0);
        }
    }
    return acc;
}

static long long gather_twice(int pairs, ...) {
    va_list ap;
    va_list copy;
    va_start(ap, pairs);
    va_copy(copy, ap);
    long long a = consume_pairs(pairs, ap);
    long long b = consume_pairs(pairs, copy);
    va_end(copy);
    va_end(ap);
    return a * 10 + b;
}

int main(void) {
    long long out = gather_twice(
        5,
        0, 11,
        1, 22u,
        2, 3333333333LL,
        3, 1.25,
        4, 9.75
    );
    printf("%lld\n", out);
    return 0;
}
