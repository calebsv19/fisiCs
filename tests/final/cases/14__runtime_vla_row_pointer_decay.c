#include <stddef.h>
#include <stdio.h>

int main(void) {
    int rows = 4;
    int cols = 3;
    int matrix[rows][cols];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix[i][j] = i * 100 + j;
        }
    }

    int (*rowp)[cols] = matrix + 1;
    int a = rowp[0][2];
    int b = rowp[2][1];
    ptrdiff_t c = &rowp[2][0] - &matrix[0][0];
    int d = (int)((rowp + 1) - matrix);

    printf("%d %d %td %d\n", a, b, c, d);
    return 0;
}
