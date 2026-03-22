#include <stddef.h>
#include <stdio.h>

static int reduce_window(int rows, int cols, int matrix[rows][cols], int base_row) {
    int (*slice)[cols] = &matrix[base_row];
    size_t u = 2;
    int s = 1;

    int v0 = slice[0][u];
    int v1 = slice[s][u - 1];
    long long row_stride = (long long)(&slice[1][0] - &slice[0][0]);
    long long elem_stride = (long long)(&slice[0][u] - &slice[0][0]);

    return v0 + v1 + (int)row_stride + (int)elem_stride;
}

int main(void) {
    int rows = 4;
    int cols = 6;
    int matrix[rows][cols];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix[i][j] = i * 100 + j * 7 + 1;
        }
    }

    int reduced = reduce_window(rows, cols, matrix, 1);
    long long block_stride = (long long)(&matrix[3][0] - &matrix[1][0]);
    int anchor = matrix[1][2] + matrix[2][1];

    printf("%d %lld %d\n", reduced, block_stride, anchor);
    return 0;
}
