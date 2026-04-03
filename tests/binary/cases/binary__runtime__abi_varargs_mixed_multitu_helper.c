#include <stdarg.h>

long long abi_var_mixed(const char *fmt, ...) {
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
        } else if (*p == 'c') {
            acc += (long long)va_arg(ap, int);
        }
        ++p;
    }
    va_end(ap);
    return acc;
}
