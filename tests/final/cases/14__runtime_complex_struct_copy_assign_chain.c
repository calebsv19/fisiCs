#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double value;
    int id;
} copy_node;

static complex_pack make_complex(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

int main(void) {
    copy_node a;
    copy_node b;
    copy_node c;
    complex_pack p = make_complex(1.50, -0.75);
    complex_pack q = make_complex(0.25, 2.25);
    complex_pack out;

    a.value = p.z;
    a.id = 4;
    b = a;
    b.value = b.value + q.z;
    b.id += 3;
    c = b;
    c.value = c.value - a.value;
    c.id += a.id;
    out.z = c.value;

    printf("%.2f %.2f | %d\n", out.lane[0], out.lane[1], c.id);
    return 0;
}
