#include <stdio.h>

static int reduce_column(int rows, int cols, int matrix[restrict static rows][cols], int col) {
    int sum = 0;
    for (int r = 0; r < rows; ++r) {
        sum += matrix[r][col];
    }
    return sum;
}

int main(void) {
    int matrix[3][5] = {
        {1, 2, 3, 4, 5},
        {6, 7, 8, 9, 10},
        {11, 12, 13, 14, 15},
    };
    int c1 = reduce_column(3, 5, matrix, 1);
    int c3 = reduce_column(3, 5, matrix, 3);
    int diag = 0;
    for (int i = 0; i < 3; ++i) {
        diag += matrix[i][i + 1];
    }
    printf("%d %d %d\n", c1, c3, diag);
    return 0;
}
