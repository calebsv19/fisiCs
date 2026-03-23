#include <stddef.h>
#include <stdio.h>

static int reduce3(int x, int y, int z, const int m[x][y][z]) {
    int sum = 0;
    for (int i = 0; i < x; ++i) {
        for (int j = 0; j < y; ++j) {
            for (int k = 0; k < z; ++k) {
                sum += m[i][j][k] * (i + 1);
            }
        }
    }
    return sum;
}

int main(void) {
    int x = 2;
    int y = 3;
    int z = 4;
    int m[x][y][z];

    for (int i = 0; i < x; ++i) {
        for (int j = 0; j < y; ++j) {
            for (int k = 0; k < z; ++k) {
                m[i][j][k] = i * 100 + j * 10 + k;
            }
        }
    }

    int total = reduce3(x, y, z, m);
    ptrdiff_t slab = &m[1][0][0] - &m[0][0][0];
    int edge = m[1][2][3];

    printf("%d %td %d\n", total, slab, edge);
    return 0;
}
