#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double value;
    int id;
} lane_node;

int main(void) {
    lane_node node;
    complex_pack out;

    node.value = (_Complex double)2.0;
    node.value = node.value - (_Complex double)3.0;
    out.z = (node.value * (_Complex double)2.0) / (_Complex double)4.0;
    node.id = 9;

    printf("%.2f %.2f %d\n", out.lane[0], out.lane[1], node.id);
    return 0;
}
