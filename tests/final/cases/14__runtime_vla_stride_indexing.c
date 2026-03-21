#include <stdio.h>

int main(void) {
    int n = 3;
    int m = 4;
    int grid[n][m];

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            grid[i][j] = i * 10 + j;
        }
    }

    int a = grid[1][3];
    int b = *(*(grid + 2) + 1);
    int c = (int)(&grid[2][0] - &grid[0][0]);

    int sum = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            sum += grid[i][j];
        }
    }

    printf("%d %d %d %d\n", a, b, c, sum);
    return 0;
}
