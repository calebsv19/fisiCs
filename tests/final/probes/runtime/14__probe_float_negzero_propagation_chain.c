#include <stdio.h>

int main(void) {
    double nz = -0.0;
    double a = nz * 2.0;
    double b = a / 3.0;
    double c = a - 0.0;

    int s1 = (1.0 / a) < 0.0;
    int s2 = (1.0 / b) < 0.0;
    int s3 = (1.0 / c) < 0.0;
    int z = (a == 0.0) && (b == 0.0) && (c == 0.0);

    printf("%d %d %d %d\n", s1, s2, s3, z);
    return 0;
}
