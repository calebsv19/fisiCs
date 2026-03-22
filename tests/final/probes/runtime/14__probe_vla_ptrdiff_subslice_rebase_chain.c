#include <stddef.h>
#include <stdio.h>

static int reduce_subslice(int rows, int cols, int matrix[rows][cols], int base_row) {
    int (*base)[cols] = &matrix[base_row];
    int (*next)[cols] = base + 1;
    size_t idx = 3;

    int lhs = base[0][idx];
    int rhs = next[1][idx - 1];
    long long row_delta = (long long)(&next[0][idx] - &base[0][idx]);
    long long rebased = (long long)(&next[1][idx - 1] - &base[0][0]);

    return lhs + rhs + (int)row_delta + (int)rebased;
}

int main(void) {
    int rows = 6;
    int cols = 7;
    int matrix[rows][cols];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix[i][j] = i * 50 + j * 5 + 2;
        }
    }

    int reduced = reduce_subslice(rows, cols, matrix, 2);
    long long block_rows = (long long)(&matrix[5][0] - &matrix[2][0]);
    int anchor = matrix[3][4];

    printf("%d %lld %d\n", reduced, block_rows, anchor);
    return 0;
}
