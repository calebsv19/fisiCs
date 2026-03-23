#include <stdio.h>

static int reduce_plane(int n, int m, int p, int q, int a[n][m][p][q], int slab, int lane) {
    int sum = 0;
    for (int i = 0; i < p; ++i) {
        sum += a[slab][lane][i][(i + lane) % q];
    }
    return sum;
}

int main(void) {
    int n = 2;
    int m = 2;
    int p = 3;
    int q = 4;
    int a[n][m][p][q];

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            for (int k = 0; k < p; ++k) {
                for (int t = 0; t < q; ++t) {
                    a[i][j][k][t] = i * 1000 + j * 100 + k * 10 + t;
                }
            }
        }
    }

    int x = reduce_plane(n, m, p, q, a, 1, 0);
    int y = reduce_plane(n, m, p, q, a, 0, 1);
    long long slab = (long long)(&a[1][0][0][0] - &a[0][0][0][0]);
    long long lane = (long long)(&a[0][1][0][0] - &a[0][0][0][0]);

    printf("%d %d %lld %lld\n", x, y, slab, lane);
    return 0;
}
