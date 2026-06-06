#include <complex.h>
#include <math.h>
#include <stdio.h>

static long scaled(double value) {
    return (long)(value * 1000.0 + (value >= 0.0 ? 0.5 : -0.5));
}

int main(void) {
    double complex upper = -4.0 + 0.0 * I;
    double complex lower = conj(upper);
    double complex root_upper = csqrt(upper);
    double complex root_lower = csqrt(lower);
    double complex logv = clog(-1.0 + 0.0 * I);
    double complex exp_log = cexp(logv);
    double complex product = (1.0 + 2.0 * I) * conj(1.0 + 2.0 * I);
    double angle = carg(-1.0 + 0.0 * I);
    long checksum = scaled(creal(root_upper)) + scaled(cimag(root_upper)) +
                    scaled(creal(root_lower)) + scaled(cimag(root_lower)) +
                    scaled(creal(logv)) + scaled(cimag(logv)) +
                    scaled(creal(exp_log)) + scaled(cimag(exp_log)) +
                    scaled(creal(product)) + scaled(cimag(product)) + scaled(angle);

    printf("complex-branch root=%ld/%ld conjroot=%ld/%ld log=%ld/%ld explog=%ld/%ld product=%ld/%ld angle=%ld checksum=%ld\n",
           scaled(creal(root_upper)),
           scaled(cimag(root_upper)),
           scaled(creal(root_lower)),
           scaled(cimag(root_lower)),
           scaled(creal(logv)),
           scaled(cimag(logv)),
           scaled(creal(exp_log)),
           scaled(cimag(exp_log)),
           scaled(creal(product)),
           scaled(cimag(product)),
           scaled(angle),
           checksum);

    return scaled(creal(root_upper)) == 0L && scaled(cimag(root_upper)) == 2000L &&
                   scaled(creal(root_lower)) == 0L && scaled(cimag(root_lower)) == -2000L &&
                   scaled(creal(logv)) == 0L && scaled(cimag(logv)) == 3142L &&
                   scaled(creal(exp_log)) == -1000L && scaled(cimag(exp_log)) == 0L &&
                   scaled(creal(product)) == 5000L && scaled(cimag(product)) == 0L &&
                   scaled(angle) == 3142L && checksum == 10284L
               ? 0
               : 1;
}
