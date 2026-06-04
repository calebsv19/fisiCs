#include <signal.h>
#include <stdio.h>

static volatile sig_atomic_t wave33_accum = 0;

static void wave33_state_handler(int signum) {
    wave33_accum = (sig_atomic_t)(wave33_accum + signum);
}

int main(void) {
    sig_atomic_t before = wave33_accum;
    if (signal(SIGINT, wave33_state_handler) == SIG_ERR) {
        return 1;
    }
    if (raise(SIGINT) != 0) {
        return 2;
    }
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        return 3;
    }
    if (raise(SIGINT) != 0) {
        return 4;
    }

    printf("signal-state before=%d after=%d sigint=%d\n", (int)before, (int)wave33_accum, SIGINT);
    return before == 0 && wave33_accum == (sig_atomic_t)SIGINT ? 0 : 1;
}
