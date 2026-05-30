#include <math.h>
#include <stdarg.h>
#include <stdio.h>

static double sum_abs_values(int count, ...) {
    va_list ap;
    double total = 0.0;
    int i = 0;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        total += fabs(va_arg(ap, double));
    }
    va_end(ap);
    return total;
}

int main(void) {
    double total = sum_abs_values(3, -1.25, 2.0, -4.0);
    double negative = -3.5;
    long total100 = (long)(total * 100.0 + 0.5);
    int sign_neg = signbit(negative) ? 1 : 0;
    int sign_abs = signbit(fabs(negative)) ? 1 : 0;

    if (total100 != 725L || !sign_neg || sign_abs) {
        return 1;
    }

    printf("sum100=%ld sign=%d/%d\n", total100, sign_neg, sign_abs);
    return 0;
}
