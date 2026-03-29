#include <limits.h>
#include <stdio.h>

static int consume_int(volatile int v) {
    return (int)v;
}

int main(void) {
    volatile int x = INT_MIN;
    volatile int y = -x; /* UB: signed overflow */

    int marker = 0;
    marker += 1;
    marker += 2;
    marker += (consume_int(y), 4);
    marker += 8;

    printf("%d\n", marker);
    return 0;
}
