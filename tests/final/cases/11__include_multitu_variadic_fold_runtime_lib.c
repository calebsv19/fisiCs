#include <stdarg.h>

#include "11__include_multitu_variadic_fold_runtime.h"

int probe_variadic_fold_inc(int base, int count, ...) {
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
