#include <stdio.h>

int main(void) {
    int n = 2;
    int m = 3;
    int p = 2;
    int q = 4;
    int vla[n][m][p][q];

    vla[0][0][0][0] = 0;
    vla[1][2][1][3] = 1213;
    vla[1][0][1][0] = 1010;

    int probe = vla[1][2][1][3];
    long long slab_stride = (long long)(&vla[1][0][0][0] - &vla[0][0][0][0]);
    long long lane_stride = (long long)(&vla[0][2][0][0] - &vla[0][0][0][0]);
    int checksum = vla[0][0][0][0] + vla[1][2][1][3] + vla[1][0][1][0];

    printf("%d %lld %lld %d\n", probe, slab_stride, lane_stride, checksum);
    return 0;
}
