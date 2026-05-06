extern int printf(const char*, ...);

int wave19_vla_static_restrict_fold(int rows, int cols, int matrix[restrict static rows][cols]);

int wave19_vla_static_restrict_fold(int rows, int cols, int (*restrict matrix)[cols]) {
    return matrix[0][2] * matrix[1][1] - matrix[2][0] + rows + cols;
}

int main(void) {
    int rows = 3;
    int cols = 4;
    int matrix[3][4] = {
        {2, 3, 5, 7},
        {11, 13, 17, 19},
        {23, 29, 31, 37},
    };
    printf("%d\n", wave19_vla_static_restrict_fold(rows, cols, matrix));
    return 0;
}
