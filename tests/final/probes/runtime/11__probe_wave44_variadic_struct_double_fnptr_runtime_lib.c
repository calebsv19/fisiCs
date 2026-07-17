#include <stdarg.h>

struct Wave44Pair {
    int x;
    int y;
};

typedef int (*Wave44Callback)(struct Wave44Pair, int);

int wave44_variadic_bridge(int seed, int count, ...) {
    va_list args;
    struct Wave44Pair left;
    struct Wave44Pair right;
    double gain;
    long long bias;
    Wave44Callback callback;
    int acc;
    int i;

    va_start(args, count);
    left = va_arg(args, struct Wave44Pair);
    gain = va_arg(args, double);
    bias = va_arg(args, long long);
    callback = va_arg(args, Wave44Callback);
    right = va_arg(args, struct Wave44Pair);
    va_end(args);

    acc = seed + count + (int)(gain * 10.0) + (int)(bias % 1000);
    for (i = 0; i < count; ++i) {
        struct Wave44Pair selected = (i & 1) ? right : left;
        acc += callback(selected, acc + i);
        acc ^= selected.x * (i + 3) + selected.y;
    }

    return acc + left.x * 3 + right.y * 5;
}
