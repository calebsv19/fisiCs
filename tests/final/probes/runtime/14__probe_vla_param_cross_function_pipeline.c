#include <stddef.h>
#include <stdio.h>

static int row_sum(int cols, const int row[cols]) {
    int acc = 0;
    for (int j = 0; j < cols; ++j) {
        acc += row[j] * (j + 1);
    }
    return acc;
}

static int matrix_sum(int rows, int cols, const int m[rows][cols]) {
    int acc = 0;
    for (int i = 0; i < rows; ++i) {
        acc += row_sum(cols, m[i]);
    }
    return acc;
}

static int slice_pipeline(int rows, int cols, int m[rows][cols], int start) {
    int (*base)[cols] = &m[start];
    int s = matrix_sum(2, cols, base);
    ptrdiff_t d = &base[1][cols - 1] - &base[0][0];
    return s + (int)d;
}

int main(void) {
    int rows = 6;
    int cols = 5;
    int m[rows][cols];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            m[i][j] = i * 30 + j * 2 + 1;
        }
    }

    int out = slice_pipeline(rows, cols, m, 2);
    ptrdiff_t tail = &m[5][4] - &m[1][1];
    printf("%d %lld\n", out, (long long)tail);
    return 0;
}

