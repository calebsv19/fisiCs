#include <signal.h>

static volatile sig_atomic_t wave33_bridge_count = 0;
static volatile sig_atomic_t wave33_bridge_last = 0;

static void wave33_bridge_handler(int signum) {
    wave33_bridge_count = (sig_atomic_t)(wave33_bridge_count + 1);
    wave33_bridge_last = (sig_atomic_t)signum;
}

int wave33_signal_action_bridge(int signum) {
    if (signal(signum, wave33_bridge_handler) == SIG_ERR) {
        return -1;
    }
    if (raise(signum) != 0) {
        return -2;
    }
    if (signal(signum, SIG_IGN) == SIG_ERR) {
        return -3;
    }
    if (raise(signum) != 0) {
        return -4;
    }
    return (int)wave33_bridge_count * 100 + (int)wave33_bridge_last;
}
