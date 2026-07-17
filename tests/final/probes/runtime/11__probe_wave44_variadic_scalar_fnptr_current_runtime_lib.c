#include <stdarg.h>

typedef int (*Wave44ScalarCallback)(int, int);

int wave44_scalar_variadic_bridge(int seed, int count, ...) {
    va_list args;
    int left;
    int right;
    double gain;
    long long bias;
    Wave44ScalarCallback callback;
    int acc;
    int i;

    va_start(args, count);
    left = va_arg(args, int);
    gain = va_arg(args, double);
    bias = va_arg(args, long long);
    callback = va_arg(args, Wave44ScalarCallback);
    right = va_arg(args, int);
    va_end(args);

    acc = seed + count + (int)(gain * 10.0) + (int)(bias % 1000);
    for (i = 0; i < count; ++i) {
        int selected = (i & 1) ? right : left;
        acc += callback(selected, acc + i);
        acc ^= selected * (i + 3);
    }

    return acc + left * 3 + right * 5;
}
