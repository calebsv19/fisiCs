#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double value;
    int which;
} select_node;

static complex_pack make_complex(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

static select_node choose(int flag, select_node a, select_node b) {
    return flag ? a : b;
}

int main(void) {
    select_node a;
    select_node b;
    select_node pick0;
    select_node pick1;
    complex_pack pa = make_complex(3.00, -2.00);
    complex_pack pb = make_complex(-1.50, 4.25);
    complex_pack out0;
    complex_pack out1;

    a.value = pa.z;
    a.which = 11;
    b.value = pb.z;
    b.which = 22;
    pick0 = choose(0, a, b);
    pick1 = choose(1, a, b);
    out0.z = pick0.value;
    out1.z = pick1.value;

    printf("%.2f %.2f | %.2f %.2f | %d %d\n",
           out0.lane[0], out0.lane[1],
           out1.lane[0], out1.lane[1],
           pick0.which, pick1.which);
    return 0;
}
