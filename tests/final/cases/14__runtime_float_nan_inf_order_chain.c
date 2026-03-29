#include <stdio.h>

int main(void) {
    double p = 1.0 / 0.0;
    double n = -1.0 / 0.0;
    double z = 0.0;
    double nan = z / z;

    int a = (p > 1e300);
    int b = (n < -1e300);
    int c = (nan != nan);
    int d = (p > n);
    int e = (nan == p);

    printf("%d %d %d %d %d\n", a, b, c, d, e);
    return 0;
}
