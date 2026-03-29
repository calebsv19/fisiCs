#include <stddef.h>
#include <stdio.h>

int abi_decay_stride_eval(int rows, int cols, const int a[rows][cols], int col);
ptrdiff_t abi_decay_row_span(int rows, int cols, const int a[rows][cols], int r0, int r1);

int main(void) {
    int rows = 5;
    int cols = 4;
    int a[rows][cols];
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            a[i][j] = i * 10 + j + 1;
        }
    }
    int s0 = abi_decay_stride_eval(rows, cols, a, 1);
    int s1 = abi_decay_stride_eval(rows, cols, a, 3);
    ptrdiff_t d = abi_decay_row_span(rows, cols, a, 1, 4);
    printf("%d %d %td\n", s0, s1, d);
    return 0;
}
