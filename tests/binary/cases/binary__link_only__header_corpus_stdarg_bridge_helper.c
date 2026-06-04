#include <stdarg.h>

long wave17_stdarg_bridge_sum(const char *spec, ...) {
    va_list ap;
    const char *p = spec;
    long total = 0;

    va_start(ap, spec);
    while (*p != '\0') {
        if (*p == 'i') {
            total += (long)va_arg(ap, int);
        } else if (*p == 'd') {
            double value = va_arg(ap, double);
            total += (long)(value * 10.0);
        } else if (*p == 'c') {
            total += (long)va_arg(ap, int);
        }
        ++p;
    }
    va_end(ap);
    return total;
}
