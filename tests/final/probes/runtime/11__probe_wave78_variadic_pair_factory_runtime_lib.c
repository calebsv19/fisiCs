#include <stdarg.h>

struct wave78_fixed {
    long seed;
    int lane;
};

struct wave78_pair {
    int a;
    int b;
};

typedef long (*wave78_variadic_fn)(struct wave78_fixed fixed, int count, ...);

static long wave78_sum(struct wave78_fixed fixed, int count, ...) {
    va_list args;
    long result = fixed.seed + fixed.lane;
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
        struct wave78_pair pair = va_arg(args, struct wave78_pair);
        result += (long)pair.a * 3 + (long)pair.b * 5 + i;
    }
    va_end(args);
    return result;
}

static long wave78_alternate(struct wave78_fixed fixed, int count, ...) {
    va_list args;
    long result = fixed.seed - fixed.lane;
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
        struct wave78_pair pair = va_arg(args, struct wave78_pair);
        result += (long)pair.a * 7 - (long)pair.b * 2 + i;
    }
    va_end(args);
    return result;
}

wave78_variadic_fn wave78_factory(int mode) {
    return (mode & 1) == 0 ? wave78_sum : wave78_alternate;
}
