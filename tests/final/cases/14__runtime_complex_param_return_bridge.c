#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

static complex_pack make_complex(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

static _Complex double combine(_Complex double lhs, _Complex double rhs) {
    return lhs + rhs - (_Complex double)1.0;
}

int main(void) {
    complex_pack a = make_complex(0.50, 3.00);
    complex_pack b = make_complex(-1.25, 2.50);
    complex_pack out;

    out.z = combine(a.z, b.z);
    printf("%.2f %.2f | %d\n", out.lane[0], out.lane[1], (out.z == out.z));
    return 0;
}
