extern int printf(const char*, ...);

int wave18_vla_const_fold(int rows, int cols, const int matrix[rows][cols]);

int wave18_vla_const_fold(int rows, int cols, const int (*matrix)[cols]) {
    return matrix[0][1] + matrix[1][0] * 2 + matrix[2][2] + rows;
}

int main(void) {
    int rows = 3;
    int cols = 3;
    const int matrix[3][3] = {
        {2, 3, 4},
        {9, 10, 11},
        {16, 17, 18},
    };
    printf("%d\n", wave18_vla_const_fold(rows, cols, matrix));
    return 0;
}
