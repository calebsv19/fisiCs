#include <stdio.h>

int main(void) {
    volatile double z = 0.0;
    volatile double n = z / z;

    int eq = (n == n);
    int ne = (n != n);
    int lt = (n < 0.0);
    int le = (n <= 0.0);
    int gt = (n > 0.0);
    int ge = (n >= 0.0);

    printf("%d %d %d %d %d %d\n", eq, ne, lt, le, gt, ge);
    return 0;
}
