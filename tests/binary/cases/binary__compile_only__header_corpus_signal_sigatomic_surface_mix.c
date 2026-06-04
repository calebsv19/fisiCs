#include <signal.h>

typedef struct {
    sig_atomic_t count;
    sig_atomic_t last;
} wave33_signal_state;

static void wave33_signal_mark(wave33_signal_state *state, int signum) {
    state->count = (sig_atomic_t)(state->count + 1);
    state->last = (sig_atomic_t)signum;
}

int main(void) {
    wave33_signal_state state = {0, 0};
    wave33_signal_mark(&state, SIGINT);
    wave33_signal_mark(&state, SIGTERM);
    return state.count == 2 && state.last == (sig_atomic_t)SIGTERM ? 0 : 1;
}
