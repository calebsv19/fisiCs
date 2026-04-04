#include <math.h>
#include <stdio.h>

int main(void) {
    double x = -42.25;
    double y = fabs(x);
    double z = fabs(-0.0);
    int sign_x = signbit(x) ? 1 : 0;
    int sign_y = signbit(y) ? 1 : 0;
    int sign_z = signbit(z) ? 1 : 0;
    long y100 = (long)(y * 100.0 + 0.5);

    if (y100 != 4225L || !sign_x || sign_y || sign_z) {
        return 1;
    }

    printf("math_fabs_signbit_roundtrip_ok y100=%ld sign=%d/%d/%d\n", y100, sign_x, sign_y, sign_z);
    return 0;
}
