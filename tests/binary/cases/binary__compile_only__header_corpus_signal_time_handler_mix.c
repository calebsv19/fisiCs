#include <signal.h>
#include <time.h>

static sig_atomic_t header_corpus_signal_seen = 0;

static void header_corpus_signal_handler(int sig) {
    header_corpus_signal_seen = (sig == SIGINT) ? 1 : -1;
}

int main(void) {
    void (*handler)(int) = header_corpus_signal_handler;
    struct tm tmv = {0};
    clock_t start = (clock_t)0;

    tmv.tm_year = 126;
    tmv.tm_mon = 4;
    tmv.tm_mday = 26;
    tmv.tm_hour = 0;
    tmv.tm_min = 0;
    tmv.tm_sec = 0;

    if (handler == 0 || start != 0) {
        return 1;
    }
    return tmv.tm_mday != 26;
}
