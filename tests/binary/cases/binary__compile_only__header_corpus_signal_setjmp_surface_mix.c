#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>

static jmp_buf g_header_corpus_wave7_jump_env;
static volatile sig_atomic_t g_header_corpus_wave7_signal_seen = 0;

static void header_corpus_wave7_mark_signal(int signum) {
    if (signum != 0) {
        g_header_corpus_wave7_signal_seen = (sig_atomic_t)signum;
    }
}

static bool header_corpus_wave7_checkpoint(void) {
    int checkpoint = setjmp(g_header_corpus_wave7_jump_env);

    if (checkpoint == 0) {
        header_corpus_wave7_mark_signal(SIGINT);
    }

    return g_header_corpus_wave7_signal_seen != 0;
}

int main(void) {
    return header_corpus_wave7_checkpoint() ? 0 : 1;
}
