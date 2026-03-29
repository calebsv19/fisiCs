#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double value;
    int code;
} inner_node;

typedef struct {
    int bias;
    inner_node inner;
} outer_node;

outer_node route_outer(outer_node in);

static complex_pack make_complex(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

int main(void) {
    outer_node in;
    outer_node out;
    complex_pack p = make_complex(5.00, -3.25);
    complex_pack r;

    in.bias = 6;
    in.inner.value = p.z;
    in.inner.code = 4;
    out = route_outer(in);
    r.z = out.inner.value;

    printf("%.2f %.2f | %d\n", r.lane[0], r.lane[1], out.inner.code);
    return 0;
}
