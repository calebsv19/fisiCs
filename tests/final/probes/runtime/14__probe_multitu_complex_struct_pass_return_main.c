#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double left;
    _Complex double right;
    int tag;
} complex_pair_block;

complex_pair_block bridge_mix(complex_pair_block in);

static complex_pack make_complex(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

int main(void) {
    complex_pair_block in;
    complex_pair_block out;
    complex_pack a = make_complex(1.00, 2.50);
    complex_pack b = make_complex(-0.25, -1.50);
    complex_pack o0;
    complex_pack o1;

    in.left = a.z;
    in.right = b.z;
    in.tag = 3;
    out = bridge_mix(in);
    o0.z = out.left;
    o1.z = out.right;

    printf("%.2f %.2f | %.2f %.2f | %d\n",
           o0.lane[0], o0.lane[1], o1.lane[0], o1.lane[1], out.tag);
    return 0;
}
