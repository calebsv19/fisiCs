#include <stddef.h>
#include <stdio.h>

static int slice_sum(int b, int c, int d, int m[b][c][d]) {
    return m[0][0][0] + m[3][1][4] + m[2][1][0];
}

int main(void) {
    int a = 3;
    int b = 4;
    int c = 2;
    int d = 5;
    int m[a][b][c][d];

    for (int i = 0; i < a; ++i) {
        for (int j = 0; j < b; ++j) {
            for (int k = 0; k < c; ++k) {
                for (int t = 0; t < d; ++t) {
                    m[i][j][k][t] = i * 1000 + j * 100 + k * 10 + t;
                }
            }
        }
    }

    int sum = slice_sum(b, c, d, m[1]);
    ptrdiff_t slab = &m[2][0][0][0] - &m[0][0][0][0];
    ptrdiff_t lane = &m[1][3][0][0] - &m[1][0][0][0];
    ptrdiff_t depth = &m[1][2][1][0] - &m[1][2][0][0];
    ptrdiff_t slice = &m[1][3][1][4] - &m[1][0][0][0];

    printf("%d %lld %lld %lld %lld\n",
           sum,
           (long long)slab,
           (long long)lane,
           (long long)depth,
           (long long)slice);
    return 0;
}
