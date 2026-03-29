#include <stdio.h>

int main(void) {
    double pz = 0.0;
    double nz = -0.0;
    double ip = 1.0 / pz;
    double in = 1.0 / nz;

    int a = (ip > 0.0);
    int b = (in < 0.0);
    int c = (pz == nz);
    int d = (ip > in);

    printf("%d %d %d %d\n", a, b, c, d);
    return 0;
}
