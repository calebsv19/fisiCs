#include <signal.h>

static volatile sig_atomic_t wave33_signal_value = 0;

static void wave33_signal_handler(int signum) {
    wave33_signal_value = (sig_atomic_t)signum;
}

int main(void) {
    void (*handler)(int) = wave33_signal_handler;
    void (*ignore_handler)(int) = SIG_IGN;
    void (*default_handler)(int) = SIG_DFL;
    int known = SIGINT + SIGTERM + SIGABRT;

    if (handler == SIG_ERR || ignore_handler == SIG_ERR || default_handler == SIG_ERR) {
        return 1;
    }
    handler(SIGTERM);
    return wave33_signal_value == (sig_atomic_t)SIGTERM && known > 0 ? 0 : 1;
}
