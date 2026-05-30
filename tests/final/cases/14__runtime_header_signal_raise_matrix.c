#include <signal.h>
#include <stdio.h>

static volatile sig_atomic_t g_header_corpus_wave7_signal_count = 0;

static void header_corpus_wave7_on_signal(int signum) {
    if (signum == SIGINT) {
        g_header_corpus_wave7_signal_count += 1;
    }
}

int main(void) {
    if (signal(SIGINT, header_corpus_wave7_on_signal) == SIG_ERR) {
        return 1;
    }
    if (raise(SIGINT) != 0) {
        return 2;
    }
    if (g_header_corpus_wave7_signal_count != 1) {
        return 3;
    }
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        return 4;
    }
    if (raise(SIGINT) != 0) {
        return 5;
    }

    printf("count=%d ignored=1\n", (int)g_header_corpus_wave7_signal_count);
    return 0;
}
