long long reduce_window(int rows, int cols, int grid[rows][cols], int row0, int row_count) {
    long long acc = 0;
    int row1 = row0 + row_count;
    for (int i = row0; i < row1; ++i) {
        for (int j = 0; j < cols; ++j) {
            acc += (long long)grid[i][j] * (long long)(i + 1) * (long long)(j + 2);
        }
    }
    return acc;
}
