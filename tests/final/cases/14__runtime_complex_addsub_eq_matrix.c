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
    complex_pack a = make_complex(1.25, -2.50);
    complex_pack b = make_complex(-0.75, 4.00);
    complex_pack sum;
    complex_pack diff;

    sum.z = a.z + b.z;
    diff.z = a.z - b.z;

    printf(
        "%.2f %.2f | %.2f %.2f | %d %d\n",
        sum.lane[0],
        sum.lane[1],
        diff.lane[0],
        diff.lane[1],
        (a.z == a.z),
        (a.z != b.z));
    return 0;
}
