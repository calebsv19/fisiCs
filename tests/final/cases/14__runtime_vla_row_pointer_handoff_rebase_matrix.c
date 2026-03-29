#include <stddef.h>
#include <stdio.h>

static int reduce_rows(int rows, int cols, int (*slice)[cols]) {
    int sum = 0;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            sum += slice[r][c] * (r + 1) + c;
        }
    }

    return sum;
}

static int edge_pick(int cols, int (*row)[cols]) {
    return (*row)[1] * 7 + (*row)[cols - 1];
}

int main(void) {
    int rows = 5;
    int cols = 4;
    int mat[rows][cols];

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            mat[r][c] = r * 13 + c * 3 + 2;
        }
    }

    int sum = reduce_rows(3, cols, &mat[1]);
    int edge = edge_pick(cols, &mat[rows - 2]);
    ptrdiff_t row_delta = &mat[4] - &mat[1];

    printf("%d %d %ld\n", sum, edge, (long)row_delta);
    return 0;
}
