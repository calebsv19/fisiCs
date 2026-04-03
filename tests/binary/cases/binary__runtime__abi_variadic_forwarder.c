#include <stdarg.h>
#include <stdio.h>

static long long fold_va(int count, va_list ap) {
    long long acc = 0;
    int i;
    for (i = 0; i < count; ++i) {
        int v = va_arg(ap, int);
        acc += (long long)(i + 1) * (long long)v;
    }
    return acc;
}

static long long forward_fold(int count, ...) {
    va_list ap;
    long long result;

    va_start(ap, count);
    result = fold_va(count, ap);
    va_end(ap);
    return result;
}

int main(void) {
    long long value = forward_fold(5, 3, 1, 4, 1, 5);
    printf("%lld\n", value);
    return 0;
}
