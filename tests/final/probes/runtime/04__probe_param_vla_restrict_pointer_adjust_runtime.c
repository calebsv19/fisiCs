extern int printf(const char*, ...);

int wave18_vla_restrict_fold(int rows, int cols, int matrix[restrict rows][cols]);

int wave18_vla_restrict_fold(int rows, int cols, int (*restrict matrix)[cols]) {
    return matrix[0][4] + matrix[1][1] * 2 + cols;
}

int main(void) {
    int rows = 2;
    int cols = 5;
    int matrix[2][5] = {
        {1, 2, 3, 4, 5},
        {10, 11, 12, 13, 14},
    };
    printf("%d\n", wave18_vla_restrict_fold(rows, cols, matrix));
    return 0;
}
