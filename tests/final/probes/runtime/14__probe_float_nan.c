#include <stdio.h>

int main(void) {
    volatile double z = 0.0;
    volatile double n = z / z;
    printf("%d\n", n != n);
    return 0;
}
