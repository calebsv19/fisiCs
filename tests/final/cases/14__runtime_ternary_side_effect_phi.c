#include <stdio.h>

int main(void) {
    int a = 0;
    int b = 5;
    int side = 1;

    int r1 = a ? (side += 100) : (side += 3);
    int r2 = b ? (side *= 2) : (side += 200);
    int r3 = (side > 6) ? (side - 1) : (side + 9);

    printf("%d %d %d %d\n", r1, r2, r3, side);
    return 0;
}
