#include <limits.h>
#include <stdio.h>

int main(void) {
    volatile int x = INT_MAX;
    int before = x;
    x += 1;

    printf("%d %d\n", before, x);
    return 0;
}
