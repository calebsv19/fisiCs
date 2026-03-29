#include <stddef.h>

int abi_decay_stride_eval(int rows, int cols, const int a[rows][cols], int col) {
    int acc = 0;
    for (int i = 0; i < rows; ++i) {
        acc += a[i][col] * (i + 2);
    }
    return acc;
}

ptrdiff_t abi_decay_row_span(int rows, int cols, const int a[rows][cols], int r0, int r1) {
    (void)rows;
    (void)cols;
    return &a[r1][0] - &a[r0][0];
}
