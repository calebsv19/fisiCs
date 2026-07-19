#include <pthread.h>

static __thread int g_depth = 7;

static void *worker(void *arg) {
    int *seen = (int *)arg;
    *seen = g_depth;
    g_depth = 19;
    return 0;
}

int main(void) {
    pthread_t thread;
    int seen = 0;

    g_depth = 11;
    if (pthread_create(&thread, 0, worker, &seen) != 0) return 1;
    if (pthread_join(thread, 0) != 0) return 2;
    if (seen != 7) return 3;
    return g_depth == 11 ? 0 : 4;
}
