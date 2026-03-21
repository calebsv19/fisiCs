#include <limits.h>
#include <stdio.h>

static int consume_int(volatile int v) {
    return (int)v;
}

int main(void) {
    volatile int a = INT_MIN;
    volatile int b = 1;
    volatile int sub_overflow = a - b; /* UB: signed overflow */

    int marker = 0;
    marker += 1;
    marker += 2;
    marker += (consume_int(sub_overflow), 4);
    marker += 8;

    printf("%d\n", marker);
    return 0;
}
