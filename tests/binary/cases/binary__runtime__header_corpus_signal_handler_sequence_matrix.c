#include <signal.h>
#include <stdio.h>

static volatile sig_atomic_t wave33_count = 0;
static volatile sig_atomic_t wave33_last = 0;

static void wave33_handler(int signum) {
    wave33_count = (sig_atomic_t)(wave33_count + 1);
    wave33_last = (sig_atomic_t)signum;
}

int main(void) {
    if (signal(SIGTERM, wave33_handler) == SIG_ERR) {
        return 1;
    }
    if (raise(SIGTERM) != 0 || raise(SIGTERM) != 0) {
        return 2;
    }
    if (signal(SIGTERM, SIG_IGN) == SIG_ERR) {
        return 3;
    }
    if (raise(SIGTERM) != 0) {
        return 4;
    }

    printf("signal-seq count=%d last=%d ignored=1\n", (int)wave33_count, (int)wave33_last);
    return wave33_count == 2 && wave33_last == (sig_atomic_t)SIGTERM ? 0 : 1;
}
