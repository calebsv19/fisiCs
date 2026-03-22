#include <stdio.h>

static volatile int g_state = 0;

static int bump(int x) {
    g_state = g_state * 5 + x;
    return g_state;
}

int main(void) {
    int a = (bump(1), bump(2));
    int b = ((bump(3) > 0) && (bump(4) > 0)) ? bump(5) : bump(6);
    int c = ((0 && bump(7)) || bump(8));
    int d = ((1 || bump(9)) && bump(10));

    printf("%d %d %d %d %d\n", a, b, c, d, (int)g_state);
    return 0;
}
