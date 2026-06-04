#include <stdarg.h>
#include <stdio.h>

static long wave17_stdarg_consume_weighted(int count, va_list ap, int base) {
    long total = 0;
    int i;

    for (i = 0; i < count; ++i) {
        total += (long)(base + i) * (long)va_arg(ap, int);
    }
    return total;
}

static long wave17_stdarg_copy_and_forward(int count, ...) {
    va_list ap;
    va_list copy;
    long first;
    long second;

    va_start(ap, count);
    va_copy(copy, ap);
    first = wave17_stdarg_consume_weighted(count, copy, 1);
    va_end(copy);
    second = wave17_stdarg_consume_weighted(count, ap, 4);
    va_end(ap);

    return first * 10 + second;
}

int main(void) {
    long total = wave17_stdarg_copy_and_forward(4, 3, 1, 4, 1);

    printf("stdarg-vacopy-forward total=%ld\n", total);
    return total == 258 ? 0 : 1;
}
