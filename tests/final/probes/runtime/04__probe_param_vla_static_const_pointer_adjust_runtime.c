extern int printf(const char*, ...);

int wave19_vla_static_const_fold(int rows, int cols, int matrix[const static rows][cols]);

int wave19_vla_static_const_fold(int rows, int cols, int (*matrix)[cols]) {
    return matrix[0][0] + matrix[1][1] + matrix[2][0] + matrix[3][1] + rows + cols;
}

int main(void) {
    int rows = 4;
    int cols = 2;
    int matrix[4][2] = {
        {3, 4},
        {7, 8},
        {11, 12},
        {15, 16},
    };
    printf("%d\n", wave19_vla_static_const_fold(rows, cols, matrix));
    return 0;
}
