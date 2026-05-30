#include <signal.h>
#include <stddef.h>

size_t header_corpus_wave7_count_digits(
    const char *text,
    volatile sig_atomic_t *digit_count_out
) {
    size_t count = 0U;

    if (!text) {
        if (digit_count_out) {
            *digit_count_out = 0;
        }
        return 0U;
    }

    while (*text) {
        if (*text >= '0' && *text <= '9') {
            count++;
        }
        text++;
    }

    if (digit_count_out) {
        *digit_count_out = (sig_atomic_t)count;
    }
    return count;
}
