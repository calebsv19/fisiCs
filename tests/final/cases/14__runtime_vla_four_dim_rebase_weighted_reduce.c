#include <stdio.h>

static int reduce_weighted(int n, int m, int p, int q, int a[n][m][p][q], int slab, int lane) {
    int sum = 0;
    for (int i = 0; i < p; ++i) {
        for (int j = 0; j < q; ++j) {
            sum += a[slab][lane][i][j] * (i + 1);
        }
    }
    return sum;
}

int main(void) {
    int n = 3;
    int m = 2;
    int p = 2;
    int q = 3;
    int a[n][m][p][q];

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            for (int k = 0; k < p; ++k) {
                for (int t = 0; t < q; ++t) {
                    a[i][j][k][t] = i * 100 + j * 10 + k * 3 + t;
                }
            }
        }
    }

    int x = reduce_weighted(n, m, p, q, a, 2, 1);
    int y = reduce_weighted(n, m, p, q, a, 0, 0);
    long long slab_delta = (long long)(&a[2][1][0][0] - &a[1][0][0][0]);
    long long lane_delta = (long long)(&a[1][1][0][0] - &a[1][0][0][0]);

    printf("%d %d %lld %lld\n", x, y, slab_delta, lane_delta);
    return 0;
}
