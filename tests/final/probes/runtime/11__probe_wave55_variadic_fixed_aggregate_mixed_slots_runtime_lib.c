#include <stdarg.h>

struct wave55_fixed_prefix {
    long stamp;
    int lane[2];
};

struct wave55_var_item {
    int left;
    int right;
};

long wave55_variadic_fixed_aggregate_mixed_slots(struct wave55_fixed_prefix prefix, int count, ...) {
    va_list ap;
    long total = prefix.stamp + prefix.lane[0] * 11 + prefix.lane[1] * 5;
    int i;

    va_start(ap, count);
    for (i = 0; i < count; i++) {
        struct wave55_var_item item = va_arg(ap, struct wave55_var_item);
        int delta = va_arg(ap, int);

        total += (long)item.left * (i + 2);
        total += (long)item.right * (delta - i);
        total += delta + prefix.lane[i & 1];
    }
    va_end(ap);
    return total;
}
