// SPDX-License-Identifier: Apache-2.0

#include <math.h>
#include <stdio.h>

int main(void) {
    int exponent = 0;
    int quotient = 0;
    double mantissa = frexp(48.0, &exponent);
    double scaled = scalbn(mantissa, exponent);
    double mod = fmod(49.0, 6.0);
    double rem = remainder(49.0, 6.0);
    double chosen = fmax(scaled, 12.0);
    double signed_value = copysign(1.0, -0.0);
    int sign = signbit(signed_value) ? 1 : 0;

    quotient = (int)((49.0 - rem) / 6.0);

    printf("canary numeric-math: exp=%d scaled=%.0f rem=%.0f quotient=%d sign=%d max=%.0f\n",
           exponent,
           scaled,
           mod,
           quotient,
           sign,
           chosen);

    return (exponent == 6 && scaled == 48.0 && mod == 1.0 && quotient == 8 && sign == 1 && chosen == 48.0) ? 0 : 1;
}
