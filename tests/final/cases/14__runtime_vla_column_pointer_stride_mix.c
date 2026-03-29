#include <stddef.h>
#include <stdio.h>

static int fold_column(int rows, int cols, int m[rows][cols], int col) {
    int sum = 0;
    int *p = &m[0][col];

    for (int r = 0; r < rows; ++r) {
        sum += (*p) * (r + 1);
        p += cols;
    }

    return sum;
}

int main(void) {
    int rows = 5;
    int cols = 7;
    int m[rows][cols];

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            m[r][c] = r * 17 + c * 5 + 1;
        }
    }

    int a = fold_column(rows, cols, m, 2);
    int b = fold_column(rows, cols, m, 5);

    int *base = &m[1][1];
    int *next = &m[4][1];
    ptrdiff_t delta = next - base;

    printf("%d %d %ld\n", a, b, (long)delta);
    return 0;
}
