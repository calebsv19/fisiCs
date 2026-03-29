#include <stdio.h>

extern long long reduce_window(int rows, int cols, int grid[rows][cols], int row0, int row_count);

int main(void) {
    int rows = 5;
    int cols = 7;
    int grid[5][7];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            grid[i][j] = i * 31 + j * 3 - 5;
        }
    }

    long long mid = reduce_window(rows, cols, grid, 1, 3);
    long long full = reduce_window(rows, cols, grid, 0, rows);
    int (*last_row)[7] = &grid[4];
    int row_delta = (int)(last_row - &grid[0]);

    printf("%lld %lld %d\n", mid, full, row_delta);
    return 0;
}
