#include <limits.h>

enum {
    wave19_limits_short_order_ok = 1 / (SHRT_MIN < 0),
    wave19_limits_int_order_ok = 1 / (INT_MIN < 0),
    wave19_limits_long_order_ok = 1 / (LONG_MIN < 0),
    wave19_limits_llong_order_ok = 1 / (LLONG_MIN < 0)
};

long long wave19_limits_integer_surface(long seed) {
    long long total = (long long)(USHRT_MAX & 0xffu);
    total += (long long)(UINT_MAX & 0xffu);
    total += (long long)(LONG_MAX > INT_MAX);
    total += (long long)(LLONG_MAX >= LONG_MAX);
    return total + (long long)seed;
}
