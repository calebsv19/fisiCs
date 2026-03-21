#include <limits.h>
#include <stdio.h>

int main(void) {
    volatile int x = INT_MIN;
    int before = x;
    --x;

    printf("%d %d\n", before, x);
    return 0;
}
