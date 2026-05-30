#include <signal.h>
#include <stddef.h>

size_t header_corpus_wave7_count_digits(
    const char *text,
    volatile sig_atomic_t *digit_count_out
);

int main(void) {
    volatile sig_atomic_t digit_count = 0;
    size_t total = header_corpus_wave7_count_digits("a1b23", &digit_count);

    return (total == (size_t)3U && digit_count == (sig_atomic_t)3) ? 0 : 1;
}
