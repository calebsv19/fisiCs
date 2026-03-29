#include <stddef.h>
#include <stdio.h>

int abi_vla_weighted_sum(int rows, int cols, int bias, int a[rows][cols]);
ptrdiff_t abi_vla_row_span(int rows, int cols, int a[rows][cols], int r0, int r1);

int main(void) {
    int rows = 4;
    int cols = 6;
    int bias = 3;
    int a[rows][cols];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            a[i][j] = i * 20 + j * 2 + 1;
        }
    }

    int s = abi_vla_weighted_sum(rows, cols, bias, a);
    ptrdiff_t d0 = abi_vla_row_span(rows, cols, a, 0, 1);
    ptrdiff_t d1 = abi_vla_row_span(rows, cols, a, 1, 3);
    printf("%d %td %td\n", s, d0, d1);
    return 0;
}
