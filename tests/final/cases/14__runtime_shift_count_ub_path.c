#include <limits.h>
#include <stdio.h>

static int consume_int(volatile int v) {
    return (int)v;
}

int main(void) {
    volatile int base = 1;
    volatile int neg_count = -1;
    volatile int wide_count = (int)(sizeof(int) * CHAR_BIT);

    volatile int left_bad = base << neg_count;   /* UB: negative shift count */
    volatile int right_bad = base << wide_count; /* UB: count >= width */

    int marker = 0;
    marker += 1;
    marker += (consume_int(left_bad), 2);
    marker += (consume_int(right_bad), 4);
    marker += 8;

    printf("%d\n", marker);
    return 0;
}
