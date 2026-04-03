#include <stdio.h>

static int mix8(int a, int b, int c, int d, int e, int f, int g, int h) {
    return (1 * a) + (2 * b) + (3 * c) + (4 * d) +
           (5 * e) + (6 * f) + (7 * g) + (8 * h);
}

int main(void) {
    int value = mix8(1, 2, 3, 4, 5, 6, 7, 8);
    printf("%d\n", value);
    return 0;
}
