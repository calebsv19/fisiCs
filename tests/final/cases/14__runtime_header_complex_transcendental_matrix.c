#include <complex.h>
#include <math.h>
#include <stdio.h>

static long scaled(double value) {
    return (long)(value * 1000.0 + (value >= 0.0 ? 0.5 : -0.5));
}

int main(void) {
    const double pi = 3.14159265358979323846;
    double complex euler = cexp(I * pi);
    double complex exp_real = cexp(log(2.0) + 0.0 * I);
    double complex log_diag = clog(1.0 + 1.0 * I);
    double complex sin_axis = csin(pi / 2.0 + 0.0 * I);
    double complex cos_axis = ccos(pi + 0.0 * I);

    long euler_r = scaled(creal(euler));
    long euler_i = scaled(cimag(euler));
    long exp_r = scaled(creal(exp_real));
    long log_r = scaled(creal(log_diag));
    long log_i = scaled(cimag(log_diag));
    long sin_r = scaled(creal(sin_axis));
    long cos_r = scaled(creal(cos_axis));
    long checksum = euler_r + euler_i + exp_r + log_r + log_i + sin_r + cos_r;

    printf("complex-trans euler=%ld/%ld exp=%ld log=%ld/%ld sin=%ld cos=%ld checksum=%ld\n",
           euler_r,
           euler_i,
           exp_r,
           log_r,
           log_i,
           sin_r,
           cos_r,
           checksum);

    return euler_r == -1000L && euler_i == 0L && exp_r == 2000L &&
                   log_r == 347L && log_i == 785L && sin_r == 1000L &&
                   cos_r == -1000L && checksum == 2132L
               ? 0
               : 1;
}
