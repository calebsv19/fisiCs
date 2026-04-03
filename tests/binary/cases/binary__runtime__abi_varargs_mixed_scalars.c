#include <stdarg.h>
#include <stdio.h>

static long long score_mixed(const char *fmt, ...) {
    va_list ap;
    const char *p = fmt;
    long long acc = 0;

    va_start(ap, fmt);
    while (*p) {
        if (*p == 'i') {
            acc += (long long)va_arg(ap, int) * 100LL;
        } else if (*p == 'd') {
            double dv = va_arg(ap, double);
            acc += (long long)(dv * 10.0);
        }
        ++p;
    }
    va_end(ap);
    return acc;
}

int main(void) {
    float f = 2.5f;
    double g = 1.75;
    int i = 3;
    printf("%lld\n", score_mixed("idid", i, f, 4, g));
    return 0;
}
