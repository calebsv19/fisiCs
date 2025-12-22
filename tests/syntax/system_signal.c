#include <signal.h>

void handler(int sig) { (void)sig; }

int main(void) {
    sigset_t set;
    struct sigaction act = {0};
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaction(SIGINT, &act, NULL);
    return 0;
}
