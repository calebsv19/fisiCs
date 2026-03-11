#include <stdio.h>

static int inc(int x) {
    return x + 1;
}

int main(void) {
    int (*fp)(int) = inc;
    printf("%d\n", fp(9));
    return 0;
}
