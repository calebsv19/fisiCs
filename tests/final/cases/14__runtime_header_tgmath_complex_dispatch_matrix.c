#include <complex.h>
#include <stdio.h>
#include <tgmath.h>

static long scaled(double value) {
    return value >= 0.0 ? (long)(value * 1000.0 + 0.5)
                        : (long)(value * 1000.0 - 0.5);
}

int main(void) {
    double complex z = 3.0 + 4.0 * I;
    double complex root = sqrt(z);
    double mag = fabs(z);
    double real_root = sqrt(9.0);

    long root_r = scaled(creal(root));
    long root_i = scaled(cimag(root));
    long mag1000 = scaled(mag);
    long real_root1000 = scaled(real_root);

    if (root_r != 2000 || root_i != 1000 || mag1000 != 5000 || real_root1000 != 3000) {
        return 1;
    }

    printf("tgmath-complex root=%ld/%ld mag=%ld real=%ld summary=%ld\n",
           root_r,
           root_i,
           mag1000,
           real_root1000,
           root_r + root_i + mag1000 + real_root1000);
    return 0;
}
