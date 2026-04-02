#include <stdio.h>

enum {
    BASE = 5,
    DELTA = 3
};

static int g_arr[BASE + DELTA * 2];
static int g_guard[(1 << 4) - 8];

int main(void) {
    unsigned long n0 = (unsigned long)(sizeof(g_arr) / sizeof(g_arr[0]));
    unsigned long n1 = (unsigned long)(sizeof(g_guard) / sizeof(g_guard[0]));
    printf("%lu %lu\n", n0, n1);
    return 0;
}
