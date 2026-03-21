#include <stddef.h>
#include <stdio.h>

static int row_dot(int cols, const int a[cols], const int b[cols]) {
    int sum = 0;
    for (int i = 0; i < cols; ++i) {
        sum += a[i] * b[i];
    }
    return sum;
}

static int matrix_reduce(int rows, int cols, const int m[rows][cols]) {
    int sum = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            sum += m[i][j] * (i + 1);
        }
    }
    return sum;
}

int main(void) {
    int rows = 3;
    int cols = 4;
    int m[rows][cols];
    int a[4] = {1, 2, 3, 4};
    int b[4] = {4, 3, 2, 1};

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            m[i][j] = i * 10 + j;
        }
    }

    int d = row_dot(cols, a, b);
    int r = matrix_reduce(rows, cols, m);
    ptrdiff_t p = &m[2][3] - &m[0][0];

    printf("%d %d %td\n", d, r, p);
    return 0;
}
