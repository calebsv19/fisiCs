#include <stddef.h>
#include <stdio.h>

static int fold_row(int cols, const int row[cols], int bias) {
    int acc = 0;
    for (int c = 0; c < cols; ++c) {
        acc += row[c] * (c + bias);
    }
    return acc;
}

static int stage_b(int rows, int cols, const int m[rows][cols], int start) {
    const int (*slice)[cols] = &m[start];
    int total = 0;
    total += fold_row(cols, slice[0], start + 1);
    total += fold_row(cols, slice[1], start + 2);
    ptrdiff_t d = &slice[1][cols - 1] - &slice[0][1];
    return total + (int)d;
}

static int stage_a(int rows, int cols, const int m[rows][cols]) {
    return stage_b(rows, cols, m, 1) + stage_b(rows, cols, m, 2);
}

int main(void) {
    int rows = 6;
    int cols = 5;
    int m[rows][cols];

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            m[r][c] = r * 19 + c * 3 + 2;
        }
    }

    int out = stage_a(rows, cols, m);
    ptrdiff_t tail = &m[5][4] - &m[0][2];
    printf("%d %lld\n", out, (long long)tail);
    return 0;
}
