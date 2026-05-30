#include <locale.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>

struct HeaderCorpusWave4Status {
    sig_atomic_t signal_count;
    char decimal;
    size_t width;
};

void header_corpus_wave4_update(struct HeaderCorpusWave4Status *state, int signum) {
    struct lconv *conv = localeconv();
    const char *decimal = ".";

    if (!state) {
        return;
    }
    if (conv && conv->decimal_point) {
        decimal = conv->decimal_point;
    }

    state->signal_count = (sig_atomic_t)signum;
    state->decimal = decimal[0];
    state->width = strlen(decimal);
}
