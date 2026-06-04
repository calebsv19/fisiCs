#include <complex.h>

static int wave31_complex_abs_arg_surface(void) {
    double complex dz = 3.0 + 4.0 * I;
    float complex fz = 1.0f - 2.0f * I;
    long double complex lz = 2.0L + 0.5L * I;
    double dm = cabs(dz);
    float fm = cabsf(fz);
    long double lm = cabsl(lz);
    double da = carg(dz);
    float fa = cargf(fz);
    long double la = cargl(lz);
    return dm > 0.0 && fm > 0.0f && lm > 0.0L && da != fa && la != 0.0L ? 0 : 1;
}

int main(void) {
    return wave31_complex_abs_arg_surface();
}
