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
    int seed;
    inner_node inner;
} outer_node;

static complex_pack make_complex(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

static outer_node step_outer(outer_node in) {
    outer_node out = in;
    out.inner.value = in.inner.value + (_Complex double)2.0;
    out.inner.code = in.inner.code + in.seed;
    return out;
}

int main(void) {
    outer_node in;
    outer_node out;
    complex_pack p = make_complex(1.75, -0.25);
    complex_pack r;

    in.seed = 5;
    in.inner.value = p.z;
    in.inner.code = 9;
    out = step_outer(in);
    r.z = out.inner.value;

    printf("%.2f %.2f | %d\n", r.lane[0], r.lane[1], out.inner.code);
    return 0;
}
