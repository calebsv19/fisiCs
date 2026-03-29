#include <stdio.h>

static int fold(int rows, int cols, int a[rows][cols]) {
    int sum = 0;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            sum += a[r][c] * (r + 1) - (c * 3);
        }
    }

    sum += (int)(sizeof a[0]);
    return sum;
}

int main(void) {
    int r1 = 3;
    int c1 = 5;
    int a1[r1][c1];
    for (int r = 0; r < r1; ++r) {
        for (int c = 0; c < c1; ++c) {
            a1[r][c] = r * 17 + c * 5 + 1;
        }
    }

    int r2 = 4;
    int c2 = 2;
    int a2[r2][c2];
    for (int r = 0; r < r2; ++r) {
        for (int c = 0; c < c2; ++c) {
            a2[r][c] = r * 11 - c * 7 + 9;
        }
    }

    int total = fold(r1, c1, a1) + fold(r2, c2, a2);
    printf("%d %lu %lu\n", total, (unsigned long)sizeof a1[0], (unsigned long)sizeof a2[0]);
    return 0;
}
