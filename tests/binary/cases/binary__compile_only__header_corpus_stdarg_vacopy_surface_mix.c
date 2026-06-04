#include <stdarg.h>

static long wave17_stdarg_take_scaled(int count, va_list ap, int scale) {
    long total = 0;
    int i;
    for (i = 0; i < count; ++i) {
        total += (long)va_arg(ap, int) * (long)scale;
    }
    return total;
}

long wave17_stdarg_vacopy_surface(int count, ...) {
    va_list ap;
    va_list copy;
    long left;
    long right;

    va_start(ap, count);
    va_copy(copy, ap);
    left = wave17_stdarg_take_scaled(count, copy, 2);
    va_end(copy);
    right = wave17_stdarg_take_scaled(count, ap, 3);
    va_end(ap);

    return left + right;
}
