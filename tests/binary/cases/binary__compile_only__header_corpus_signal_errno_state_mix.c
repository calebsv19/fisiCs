#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>

struct HeaderCorpusSignalState {
    sig_atomic_t seen;
    int err;
};

static void header_corpus_wave4_ignore(int signo) {
    (void)signo;
}

int header_corpus_signal_errno_state_mix(struct HeaderCorpusSignalState *state, const char *text) {
    char *end = 0;
    long parsed = 0;

    if (!state || !text) {
        return -1;
    }

    (void)signal(SIGINT, header_corpus_wave4_ignore);
    errno = 0;
    parsed = strtol(text, &end, 10);
    state->seen = (sig_atomic_t)(end != text);
    state->err = errno;
    return (int)(offsetof(struct HeaderCorpusSignalState, err) + parsed);
}
