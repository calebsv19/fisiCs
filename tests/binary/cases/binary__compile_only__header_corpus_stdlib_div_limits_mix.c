#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

static bool header_corpus_wave8_div_window(
    long numerator,
    long denominator,
    ldiv_t *pair_out,
    size_t *span_out
) {
    static const int values[] = {2, 4, 6, 8};

    if (!pair_out || !span_out || denominator == 0L) {
        return false;
    }

    *pair_out = ldiv(numerator, denominator);
    *span_out = (size_t)(&values[3] - &values[0]);
    return numerator <= LONG_MAX;
}

int main(void) {
    ldiv_t pair;
    size_t span = 0U;

    return header_corpus_wave8_div_window(17L, 3L, &pair, &span) ? 0 : 1;
}
