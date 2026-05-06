int wave19_multitu_vla_static_fold(int rows, int cols, int (*matrix)[cols]) {
    return matrix[0][3] + matrix[1][2] + rows * cols;
}
