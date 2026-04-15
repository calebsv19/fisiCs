#include <stdio.h>

typedef struct TimerBackend {
    int marker;
} TimerBackend;

static const TimerBackend* g_backend = 0;
static TimerBackend g_storage = {7};

static int register_backend(void) {
    g_backend = &g_storage;
    if (!g_backend) {
        return -1;
    }
    return g_backend->marker;
}

int main(void) {
    int value = register_backend();
    if (value != 7) {
        printf("bad:%d\n", value);
        return 1;
    }
    puts("ok");
    return 0;
}
