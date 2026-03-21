#include <stddef.h>
#include <stdio.h>

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

    int edge = m[1][2][3];
    ptrdiff_t slab = &m[1][0][0] - &m[0][0][0];
    ptrdiff_t lane = &m[0][2][0] - &m[0][0][0];

    printf("%d %td %td\n", edge, slab, lane);
    return 0;
}
