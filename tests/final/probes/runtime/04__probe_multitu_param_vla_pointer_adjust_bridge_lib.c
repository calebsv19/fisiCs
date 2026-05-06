int wave18_multitu_vla_fold(int rows, int cols, int (*matrix)[cols]) {
    return matrix[1][2] + matrix[2][0] + matrix[3][1];
}
