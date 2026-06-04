#include <stdarg.h>
#include <stdio.h>

static long wave17_stdarg_collect(const char *spec, ...) {
    va_list ap;
    const char *p = spec;
    long total = 0;

    va_start(ap, spec);
    while (*p != '\0') {
        if (*p == 'c') {
            total += (long)va_arg(ap, int);
        } else if (*p == 's') {
            total += (long)va_arg(ap, int);
        } else if (*p == 'i') {
            total += (long)va_arg(ap, int);
        } else if (*p == 'f') {
            double value = va_arg(ap, double);
            total += (long)(value * 100.0);
        }
        ++p;
    }
    va_end(ap);
    return total;
}

int main(void) {
    char small = 5;
    short medium = 12;
    float scaled = 2.25f;
    long total = wave17_stdarg_collect("csfi", small, medium, scaled, 19);

    printf("stdarg-promotions total=%ld\n", total);
    return total == 261 ? 0 : 1;
}
