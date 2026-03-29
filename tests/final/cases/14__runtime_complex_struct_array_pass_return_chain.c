#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double lanes[2];
    int tag;
} complex_block;

static complex_pack make_complex(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

static complex_block transform_block(complex_block in) {
    complex_block out = in;
    out.lanes[0] = in.lanes[0] + in.lanes[1];
    out.lanes[1] = in.lanes[1] - in.lanes[0];
    out.tag = in.tag + 3;
    return out;
}

int main(void) {
    complex_block in;
    complex_block out;
    complex_pack a = make_complex(2.00, -1.50);
    complex_pack b = make_complex(-0.25, 0.75);
    complex_pack out0;
    complex_pack out1;

    in.lanes[0] = a.z;
    in.lanes[1] = b.z;
    in.tag = 7;
    out = transform_block(in);
    out0.z = out.lanes[0];
    out1.z = out.lanes[1];

    printf("%.2f %.2f | %.2f %.2f | %d\n",
           out0.lane[0], out0.lane[1], out1.lane[0], out1.lane[1], out.tag);
    return 0;
}
