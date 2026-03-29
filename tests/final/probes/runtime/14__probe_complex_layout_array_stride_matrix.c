#include <stdio.h>
#include <stddef.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    _Complex double value;
    int tag;
} row_layout;

int main(void) {
    row_layout rows[3];
    ptrdiff_t stride = (char*)&rows[1] - (char*)&rows[0];
    complex_pack out;

    rows[0].value = (_Complex double)1.0;
    rows[1].value = (_Complex double)2.5;
    rows[2].value = (_Complex double)-0.75;
    rows[0].tag = 3;
    rows[1].tag = 5;
    rows[2].tag = 7;
    out.z = rows[0].value + rows[1].value + rows[2].value;

    printf("%td %zu %.2f %.2f %d\n",
           stride, sizeof(row_layout), out.lane[0], out.lane[1],
           rows[0].tag + rows[1].tag + rows[2].tag);
    return 0;
}
