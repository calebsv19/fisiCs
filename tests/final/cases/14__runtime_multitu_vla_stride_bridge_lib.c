#include <stddef.h>

int abi_vla_weighted_sum(int rows, int cols, int bias, int a[rows][cols]) {
    int acc = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int w = (i + 1) * (j + 2) + bias;
            acc += a[i][j] * w;
        }
    }
    return acc;
}

ptrdiff_t abi_vla_row_span(int rows, int cols, int a[rows][cols], int r0, int r1) {
    (void)rows;
    (void)cols;
    return &a[r1][0] - &a[r0][0];
}
