#include <stdio.h>

typedef int (*Wave44ScalarCallback)(int, int);

int wave44_scalar_variadic_bridge(int seed, int count, ...);

static int scalar_callback(int value, int salt) {
    return value * 5 + (salt % 23);
}

int main(void) {
    int folded = wave44_scalar_variadic_bridge(
        13,
        4,
        17,
        2.5,
        10000000037LL,
        scalar_callback,
        41);

    printf("%d\n", folded);
    return 0;
}
