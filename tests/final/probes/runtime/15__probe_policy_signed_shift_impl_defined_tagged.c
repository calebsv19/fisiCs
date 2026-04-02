#include <stdio.h>

int main(void) {
    volatile int base = -19;
    volatile int s1 = base >> 1;
    volatile int s2 = base >> 3;
    printf("%d %d\n", s1, s2);
    return 0;
}
