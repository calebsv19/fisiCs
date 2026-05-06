extern int printf(const char*, ...);

int wave18_vla_fold(int rows, int cols, int matrix[rows][cols]);

int wave18_vla_fold(int rows, int cols, int (*matrix)[cols]) {
    return matrix[0][0] + matrix[1][2] + matrix[2][1] + rows + cols;
}

int main(void) {
    int rows = 3;
    int cols = 4;
    int matrix[3][4] = {
        {1, 2, 3, 4},
        {11, 12, 13, 14},
        {21, 22, 23, 24},
    };
    printf("%d\n", wave18_vla_fold(rows, cols, matrix));
    return 0;
}
