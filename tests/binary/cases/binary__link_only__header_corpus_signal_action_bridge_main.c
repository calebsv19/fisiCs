#include <signal.h>

int wave33_signal_action_bridge(int signum);

int main(void) {
    int score = wave33_signal_action_bridge(SIGTERM);
    return score == (100 + SIGTERM) ? 0 : 1;
}
