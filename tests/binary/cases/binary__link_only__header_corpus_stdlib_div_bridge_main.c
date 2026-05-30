#include <stdlib.h>

ldiv_t header_corpus_wave8_ldiv_bridge(long numerator, long denominator);

int main(void) {
    ldiv_t pair = header_corpus_wave8_ldiv_bridge(17L, 3L);

    return (pair.quot == 5L && pair.rem == 2L) ? 0 : 1;
}
