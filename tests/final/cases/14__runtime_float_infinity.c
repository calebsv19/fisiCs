#include <stdio.h>

int main(void) {
    volatile double z = 0.0;
    volatile double x = 1.0 / z;
    printf("%d\n", x > 1e300);
    return 0;
}
