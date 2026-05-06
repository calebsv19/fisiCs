extern int printf(const char*, ...);

int wave18_multitu_vla_fold(int rows, int cols, int matrix[rows][cols]);

int main(void) {
    int rows = 4;
    int cols = 3;
    int matrix[4][3] = {
        {2, 3, 4},
        {7, 8, 9},
        {12, 13, 14},
        {17, 18, 19},
    };
    printf("%d\n", wave18_multitu_vla_fold(rows, cols, matrix));
    return 0;
}
