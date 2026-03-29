#include <stddef.h>
#include <stdio.h>

static int reduce_col(int n, int m, int a[n][m], int col) {
    int sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += a[i][col] * (i + 1);
    }
    return sum;
}

static ptrdiff_t row_span(int n, int m, int a[n][m], int r0, int r1) {
    return &a[r1][0] - &a[r0][0];
}

int main(void) {
    int n = 4;
    int m = 5;
    int a[n][m];

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            a[i][j] = i * 10 + j;
        }
    }

    int s1 = reduce_col(n, m, a, 2);
    int s2 = reduce_col(n, m, a, 4);
    ptrdiff_t d1 = row_span(n, m, a, 0, 3);
    ptrdiff_t d2 = &a[3][4] - &a[0][1];

    printf("%d %d %td %td\n", s1, s2, d1, d2);
    return 0;
}
