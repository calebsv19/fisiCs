#include <stdarg.h>

int probe_variadic_fold(int base, int count, ...) {
    va_list ap;
    int total = base;
    int i;
    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        total += va_arg(ap, int);
    }
    va_end(ap);
    return total;
}
