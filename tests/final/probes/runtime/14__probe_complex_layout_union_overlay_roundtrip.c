#include <stdio.h>

typedef union {
    _Complex double z;
    double lane[2];
} complex_pack;

typedef struct {
    complex_pack first;
    complex_pack second;
} pair_struct;

typedef union {
    pair_struct pair;
    complex_pack flat[2];
} overlay_union;

int main(void) {
    overlay_union u;
    complex_pack sum;

    u.flat[0].lane[0] = 1.25;
    u.flat[0].lane[1] = -2.50;
    u.flat[1].lane[0] = 0.75;
    u.flat[1].lane[1] = 3.25;
    sum.z = u.pair.first.z + u.pair.second.z;

    printf("%.2f %.2f | %.2f %.2f\n",
           u.pair.first.lane[0], u.pair.first.lane[1],
           sum.lane[0], sum.lane[1]);
    return 0;
}
