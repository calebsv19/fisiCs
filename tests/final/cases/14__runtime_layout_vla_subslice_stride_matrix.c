#include <stddef.h>
#include <stdio.h>

int main(void) {
    int n = 3;
    int m = 4;
    int k = 5;
    int a[n][m][k];

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            for (int t = 0; t < k; ++t) {
                a[i][j][t] = i * 100 + j * 10 + t;
            }
        }
    }

    ptrdiff_t d_row = &a[1][0][0] - &a[0][0][0];
    ptrdiff_t d_col = &a[0][1][0] - &a[0][0][0];
    ptrdiff_t d_elem = &a[0][0][4] - &a[0][0][0];

    int k1 = (d_row == (ptrdiff_t)(m * k));
    int k2 = (d_col == (ptrdiff_t)k);
    int k3 = (d_elem == 4);

    int reduce = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            reduce += a[i][j][(i + j) % k];
        }
    }

    printf("%d %d %d %d\n", k1, k2, k3, reduce);
    return 0;
}
