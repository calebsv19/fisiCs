#include <stddef.h>
#include <stdio.h>

int main(void) {
    int rows = 6;
    int cols = 5;
    int matrix[rows][cols];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix[i][j] = i * 10 + j;
        }
    }

    int (*p)[cols] = matrix + 2;
    int (*q)[cols] = matrix + 5;

    ptrdiff_t d1 = q - p;
    ptrdiff_t d2 = (p + 2) - matrix;
    ptrdiff_t d3 = &q[0][4] - &p[0][1];
    int sample = p[1][3] + q[-1][0];

    printf("%td %td %td %d\n", d1, d2, d3, sample);
    return 0;
}
