#include <stddef.h>
#include <stdio.h>

static int reduce3(int x, int y, int z, int a[x][y][z]) {
    int sum = 0;
    for (int i = 0; i < x; ++i) {
        for (int j = 0; j < y; ++j) {
            int k = (i + j * 2) % z;
            sum += a[i][j][k];
        }
    }
    return sum;
}

int main(void) {
    int x = 3;
    int y = 4;
    int z = 6;
    int a[x][y][z];

    for (int i = 0; i < x; ++i) {
        for (int j = 0; j < y; ++j) {
            for (int k = 0; k < z; ++k) {
                a[i][j][k] = i * 100 + j * 10 + k;
            }
        }
    }

    ptrdiff_t dx = &a[1][0][0] - &a[0][0][0];
    ptrdiff_t dy = &a[0][1][0] - &a[0][0][0];
    ptrdiff_t dz = &a[0][0][5] - &a[0][0][0];

    int ok = 0;
    if (dx == (ptrdiff_t)(y * z)) ok += 1;
    if (dy == (ptrdiff_t)z) ok += 1;
    if (dz == 5) ok += 1;

    int r = reduce3(x, y, z, a);
    printf("%td %td %td %d %d\n", dx, dy, dz, ok, r);
    return 0;
}
