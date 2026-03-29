int abi_vla_reduce(int rows, int cols, int matrix[rows][cols]) {
    int sum = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            sum += matrix[r][c] * (r + 1) - c;
        }
    }
    return sum;
}

