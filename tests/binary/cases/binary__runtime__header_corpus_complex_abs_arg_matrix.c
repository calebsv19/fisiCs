#include <complex.h>
#include <stdio.h>

static long scaled(double value) {
    return (long)(value * 1000.0 + (value >= 0.0 ? 0.5 : -0.5));
}

int main(void) {
    double complex a = 3.0 + 4.0 * I;
    double complex b = 0.0 + 1.0 * I;
    double complex c = -1.0 + 0.0 * I;
    long mag_a = scaled(cabs(a));
    long mag_c = scaled(cabs(c));
    long arg_b = scaled(carg(b));
    long arg_c = scaled(carg(c));

    printf("complex-abs-arg mag=%ld/%ld arg=%ld/%ld\n", mag_a, mag_c, arg_b, arg_c);
    return mag_a == 5000L && mag_c == 1000L && arg_b == 1571L && arg_c == 3142L ? 0 : 1;
}
