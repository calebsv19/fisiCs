#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double left;
    _Complex double right;
} complex_pair;

static complex_pack make_complex(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

static complex_pair transform_pair(complex_pair in) {
    complex_pair out;
    out.left = in.left + in.right;
    out.right = in.right - in.left;
    return out;
}

int main(void) {
    complex_pack left = make_complex(0.50, 3.00);
    complex_pack right = make_complex(-1.25, 2.50);
    complex_pair input;
    complex_pair output;
    complex_pack out_left;
    complex_pack out_right;

    input.left = left.z;
    input.right = right.z;
    output = transform_pair(input);
    out_left.z = output.left;
    out_right.z = output.right;

    printf(
        "%.2f %.2f | %.2f %.2f\n",
        out_left.lane[0],
        out_left.lane[1],
        out_right.lane[0],
        out_right.lane[1]);
    return 0;
}
