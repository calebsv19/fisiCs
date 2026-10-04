// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <string.h>
#include <complex.h>

static float global_hex = 0x1.fp0f;
static float global_decimal = 1.0000000596046448f;
static double global_widened = 1.0000000596046448f;
static float global_unsuffixed = 1.0000000596046448;
static float global_folded = 1.0000000596046448f + 0.0f;
static double global_cast = (double)1.0000000596046448f;
static float aggregate[] = {0x1.fP0F, 1.0000000596046448F, 0x1p-149f};
static unsigned bits(float f) { unsigned u; memcpy(&u, &f, sizeof u); return u; }
static unsigned long long wide_bits(double d) { unsigned long long u; memcpy(&u, &d, sizeof u); return u; }
int main(void) {
    volatile float hex = 0x1.fp0f;
    volatile float upper = 0X1.FFP+0F;
    volatile double wide = 0x1.fp0;
    volatile long double extended = 0x1.fp0L;
    volatile float decimal = 1.0000000596046448f;
    volatile float unsuffixed = 1.0000000596046448;
    volatile float long_spelling = 1.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001e0f;
    volatile float subnormal = 0x1p-149f;
    static float local_static = 1.0000000596046448f;
    float _Complex imaginary = 0x1.fp0fi;
    float _Complex forward = 1.0000000596046448fi;
    float _Complex reverse = 1.0000000596046448iF;
    float _Complex upper_forward = 0X1.FP0FJ;
    float _Complex upper_reverse = 0X1.FP0JF;
    long double _Complex long_reverse = 1.0iL;
    printf("hex=%08x upper=%08x double=%016llx long-double=%016llx\n", bits(hex), bits(upper), wide_bits(wide), wide_bits((double)extended));
    printf("decimal=%08x unsuffixed=%08x long=%08x subnormal=%08x imaginary=%08x\n", bits(decimal), bits(unsuffixed), bits(long_spelling), bits(subnormal), bits(cimagf(imaginary)));
    printf("globals=%08x,%08x,%016llx,%08x,%08x,%016llx static=%08x\n",bits(global_hex),bits(global_decimal),wide_bits(global_widened),bits(global_unsuffixed),bits(global_folded),wide_bits(global_cast),bits(local_static));
    printf("aggregate=%08x,%08x,%08x\n",bits(aggregate[0]),bits(aggregate[1]),bits(aggregate[2]));
    printf("imaginary-orders=%08x,%08x,%08x,%08x types=%d/%d/%d\n",
           bits(cimagf(forward)), bits(cimagf(reverse)), bits(cimagf(upper_forward)), bits(cimagf(upper_reverse)),
           (int)(sizeof(1.0fi) == sizeof(float _Complex)),
           (int)(sizeof(1.0iF) == sizeof(float _Complex)),
           (int)(sizeof(1.0iL) == sizeof(long double _Complex) && cimagl(long_reverse) == 1.0L));
    float _Complex small = 5.0f + 12.0fi;
    double _Complex mixed = 16777216.0 + small;
    double _Complex reverse_mixed = small + 16777216.0;
    double _Complex product = 3.0 * small;
    double _Complex quotient = 5.0fi / 2.0;
    printf("complex-promotions=%d/%d/%d/%d\n",
           (int)(creal(mixed) == 16777221.0 && cimag(mixed) == 12.0),
           (int)(creal(reverse_mixed) == 16777221.0 && cimag(reverse_mixed) == 12.0),
           (int)(creal(product) == 15.0 && cimag(product) == 36.0),
           (int)(creal(quotient) == 0.0 && cimag(quotient) == 2.5));
    return 0;
}
