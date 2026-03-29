#include <stdio.h>

int main(void) {
    volatile long long neg = -33;
    volatile long long r1 = neg >> 1;
    volatile long long r2 = neg >> 4;
    volatile char c = (char)200;
    int cneg = (c < 0);

    printf("%lld %lld %d\n", r1, r2, cneg);
    return 0;
}
