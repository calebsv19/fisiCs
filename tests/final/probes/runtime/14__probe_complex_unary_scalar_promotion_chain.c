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
    complex_pack base = make_complex(2.50, -1.25);
    complex_pack promoted;
    complex_pack negated;

    promoted.z = base.z + 3.00;
    negated.z = -base.z;

    printf(
        "%.2f %.2f | %.2f %.2f | %d %d\n",
        promoted.lane[0],
        promoted.lane[1],
        negated.lane[0],
        negated.lane[1],
        (base.z == base.z),
        (base.z != promoted.z));
    return 0;
}
