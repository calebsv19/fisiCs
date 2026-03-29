#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double value;
    int count;
} cell_node;

static complex_pack make_complex(double real, double imag) {
    complex_pack out;
    out.lane[0] = real;
    out.lane[1] = imag;
    return out;
}

static cell_node read_cell(const cell_node* node) {
    return *node;
}

int main(void) {
    cell_node cells[2];
    complex_pack a = make_complex(4.50, -1.00);
    complex_pack b = make_complex(-2.25, 3.00);
    complex_pack r0;
    complex_pack r1;

    cells[0].value = a.z;
    cells[0].count = 1;
    cells[1].value = b.z;
    cells[1].count = 5;
    cells[0] = read_cell(&cells[0]);
    cells[1] = read_cell(&cells[1]);
    r0.z = cells[0].value + (_Complex double)1.25;
    r1.z = cells[1].value - (_Complex double)0.75;

    printf("%.2f %.2f | %.2f %.2f | %d %d\n",
           r0.lane[0], r0.lane[1], r1.lane[0], r1.lane[1],
           cells[0].count, cells[1].count);
    return 0;
}
