#include <stdarg.h>
#include <stdio.h>

static long long fold(const char *fmt, ...) {
    va_list ap;
    long long acc = 0;
    const char *p = fmt;

    va_start(ap, fmt);
    while (*p) {
        if (*p == 'i') {
            acc += (long long)va_arg(ap, int) * 1000LL;
        } else if (*p == 'd') {
            double dv = va_arg(ap, double);
            acc += (long long)(dv * 100.0 + 0.5);
        } else if (*p == 'c') {
            acc += (long long)va_arg(ap, int) * 10LL;
        } else if (*p == 's') {
            acc += (long long)va_arg(ap, int);
        }
        ++p;
    }
    va_end(ap);
    return acc;
}

int main(void) {
    float f = 1.25f;
    char c = 3;
    short s = 5;
    long long value = fold("idcs", 11, f, c, s);
    printf("%lld\n", value);
    return 0;
}
