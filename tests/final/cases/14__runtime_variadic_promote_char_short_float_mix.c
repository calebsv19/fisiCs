#include <stdio.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

static long long collect(int tags, va_list ap) {
    long long acc = 0;
    for (int i = 0; i < tags; ++i) {
        int tag = va_arg(ap, int);
        if (tag == 0) {
            int c = va_arg(ap, int);
            acc += (long long)c * 11;
        } else if (tag == 1) {
            int s = va_arg(ap, int);
            acc += (long long)s * 7;
        } else if (tag == 2) {
            double f = va_arg(ap, double);
            acc += (long long)(f * 100.0);
        } else {
            unsigned int u = va_arg(ap, unsigned int);
            acc += (long long)u;
        }
    }
    return acc;
}

static long long gather(int tags, ...) {
    va_list ap;
    va_list cp;
    va_start(ap, tags);
    va_copy(cp, ap);
    long long a = collect(tags, ap);
    long long b = collect(tags, cp);
    va_end(cp);
    va_end(ap);
    return a + b * 10;
}

int main(void) {
    signed char c = -5;
    short s = 300;
    float f = 1.75f;
    unsigned char u = 250u;
    long long out = gather(4, 0, c, 1, s, 2, f, 3, u);
    printf("%lld\n", out);
    return 0;
}
