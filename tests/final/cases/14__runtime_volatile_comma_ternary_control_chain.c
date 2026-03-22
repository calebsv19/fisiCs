#include <stdio.h>

static volatile int g_state = 0;

static int step(int x) {
    g_state = g_state * 7 + x;
    return g_state;
}

int main(void) {
    int a = (step(1), step(2));
    int b = (step(3) > 0) ? step(4) : step(5);
    int c = ((step(6), 0) ? step(7) : step(8));
    int d = ((1 || step(9)) && (step(10) > 0));

    printf("%d %d %d %d %d\n", a, b, c, d, (int)g_state);
    return 0;
}
