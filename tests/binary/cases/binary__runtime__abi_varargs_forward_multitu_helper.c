#include <stdarg.h>

static long long abi_sum_weighted_scaled(int count, va_list ap) {
    long long acc = 0;
    int i;
    for (i = 0; i < count; ++i) {
        double v = va_arg(ap, double);
        long long scaled = (long long)(v * 4.0);
        acc += (long long)(i + 1) * scaled;
    }
    return acc;
}

long long abi_var_forward_scaled(int count, ...) {
    va_list ap;
    long long result;
    va_start(ap, count);
    result = abi_sum_weighted_scaled(count, ap);
    va_end(ap);
    return result;
}
