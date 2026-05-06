extern int printf(const char*, ...);

int wave19_multitu_vla_static_fold(int rows, int cols, int matrix[static rows][cols]);

int main(void) {
    int rows = 2;
    int cols = 4;
    int matrix[2][4] = {
        {1, 3, 5, 7},
        {2, 4, 6, 8},
    };
    printf("%d\n", wave19_multitu_vla_static_fold(rows, cols, matrix));
    return 0;
}
