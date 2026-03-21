#include <limits.h>
#include <stdio.h>

static int consume_int(volatile int v) {
    return (int)v;
}

int main(void) {
    volatile int a = INT_MAX;
    volatile int b = 1;
    volatile int add_overflow = a + b; /* UB: signed overflow */

    volatile int m = INT_MIN;
    volatile int negate_overflow = -m; /* UB: INT_MIN negation */

    int marker = 0;
    marker += 1;
    marker += 2;
    marker += (consume_int(add_overflow), 4);
    marker += (consume_int(negate_overflow), 8);

    printf("%d\n", marker);
    return 0;
}
