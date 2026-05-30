#include <locale.h>
#include <signal.h>
#include <stddef.h>

struct HeaderCorpusWave4Status {
    sig_atomic_t signal_count;
    char decimal;
    size_t width;
};

void header_corpus_wave4_update(struct HeaderCorpusWave4Status *state, int signum);

int main(void) {
    struct HeaderCorpusWave4Status state = {0};

    header_corpus_wave4_update(&state, SIGTERM);
    return (int)(state.width + (size_t)state.signal_count + (size_t)state.decimal);
}
