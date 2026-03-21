#include <stdio.h>

int main(void) {
    volatile int neg = -8;
    volatile int s1 = neg >> 1;
    volatile int s2 = neg >> 2;
    volatile int pos = 8 >> 1;

    int sign1 = (s1 < 0);
    int sign2 = (s2 < 0);

    printf("%d %d %d %d %d\n", s1, s2, pos, sign1, sign2);
    return 0;
}
