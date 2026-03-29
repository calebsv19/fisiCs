#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

complex_pack complex_bridge_pack(double real, double imag);
_Complex double complex_bridge_mix(_Complex double a, _Complex double b);

int main(void) {
    complex_pack a = complex_bridge_pack(4.50, -3.00);
    complex_pack b = complex_bridge_pack(-1.00, 1.25);
    complex_pack out;

    out.z = complex_bridge_mix(a.z, b.z);
    printf("%.2f %.2f %d\n", out.lane[0], out.lane[1], (out.z == out.z));
    return 0;
}
