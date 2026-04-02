#include <stdio.h>

static int lane(int seed) {
    volatile int top = 0x7fffffff;
    int a = top + seed;
    int b = a + seed;
    return b ^ (a << 1);
}

int main(void) {
    int p = lane(1);
    int q = lane(3);
    printf("%d %d\n", p, q);
    return 0;
}
