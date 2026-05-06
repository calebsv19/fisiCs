extern int printf(const char*, ...);

int wave19_vla_static_fold(int rows, int cols, int matrix[static rows][cols]);

int wave19_vla_static_fold(int rows, int cols, int (*matrix)[cols]) {
    return matrix[0][1] + matrix[1][0] + matrix[2][2] + rows + cols;
}

int main(void) {
    int rows = 3;
    int cols = 3;
    int matrix[3][3] = {
        {1, 2, 3},
        {10, 11, 12},
        {20, 21, 22},
    };
    printf("%d\n", wave19_vla_static_fold(rows, cols, matrix));
    return 0;
}
