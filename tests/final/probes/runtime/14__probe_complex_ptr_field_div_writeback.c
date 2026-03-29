#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double value;
    int tag;
} node_t;

int main(void) {
    node_t n;
    node_t* p = &n;
    complex_pack out;

    p->value = (_Complex double)5.0;
    p->value /= (_Complex double)2.0;
    p->value /= (_Complex double)5.0;
    p->tag = 13;
    out.z = p->value;

    printf("%.2f %.2f %d\n", out.lane[0], out.lane[1], p->tag);
    return 0;
}
