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

int main(void) {
    complex_pack a = make_complex(1.50, 2.00);
    complex_pack b = make_complex(-0.50, 4.00);
    complex_pack prod;
    complex_pack quot;

    prod.z = a.z * b.z;
    quot.z = a.z / b.z;

    printf("%.2f %.2f | %.2f %.2f\n",
           prod.lane[0], prod.lane[1],
           quot.lane[0], quot.lane[1]);
    return 0;
}
