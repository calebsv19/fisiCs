#include <math.h>
#include <stdarg.h>
#include <stdio.h>

static double header_corpus_abs_sum(int count, ...) {
    va_list ap;
    double total = 0.0;
    int i = 0;

    va_start(ap, count);
    for (i = 0; i < count; ++i) {
        double value = va_arg(ap, double);
        total += fabs(value);
    }
    va_end(ap);
    return total;
}

static int header_corpus_bridge(FILE* stream) {
    char buffer[32];
    double total = header_corpus_abs_sum(3, -1.0, 2.5, -3.25);
    int sign = signbit(-0.0) ? 1 : 0;
    int wrote = snprintf(buffer, sizeof(buffer), "%.2f/%d", total, sign);
    return (stream != 0 && wrote > 0) ? 0 : 1;
}

int main(void) {
    return header_corpus_bridge(stdout);
}
