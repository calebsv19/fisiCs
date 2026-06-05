#include <complex.h>
#include <math.h>
#include <stdio.h>

static long scaled(double value) {
    return (long)(value * 1000.0 + (value >= 0.0 ? 0.5 : -0.5));
}

int main(void) {
    double complex root = csqrt(3.0 + 4.0 * I);
    double complex square = cpow(1.0 + 1.0 * I, 2.0 + 0.0 * I);
    double complex cube = cpow(2.0 + 0.0 * I, 3.0 + 0.0 * I);
    double complex projected = cproj(3.0 - 4.0 * I);
    double hyp = cabs(5.0 + 12.0 * I);

    long root_r = scaled(creal(root));
    long root_i = scaled(cimag(root));
    long square_r = scaled(creal(square));
    long square_i = scaled(cimag(square));
    long cube_r = scaled(creal(cube));
    long proj_r = scaled(creal(projected));
    long proj_i = scaled(cimag(projected));
    long hyp_r = scaled(hyp);
    long checksum = root_r + root_i + square_r + square_i + cube_r + proj_r + proj_i + hyp_r;

    printf("complex-power root=%ld/%ld square=%ld/%ld cube=%ld proj=%ld/%ld hyp=%ld checksum=%ld\n",
           root_r,
           root_i,
           square_r,
           square_i,
           cube_r,
           proj_r,
           proj_i,
           hyp_r,
           checksum);

    return root_r == 2000L && root_i == 1000L && square_r == 0L &&
                   square_i == 2000L && cube_r == 8000L && proj_r == 3000L &&
                   proj_i == -4000L && hyp_r == 13000L && checksum == 25000L
               ? 0
               : 1;
}
