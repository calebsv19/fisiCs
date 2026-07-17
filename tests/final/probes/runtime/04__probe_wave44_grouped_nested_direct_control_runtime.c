#include <stdio.h>

enum {
    WAVE44_PLANES = 2,
    WAVE44_WIDTH = 3
};

int wave44_grid_score(
    int (*grid)[WAVE44_PLANES][WAVE44_WIDTH]);

int wave44_grid_score(
    int (*grid)[sizeof(char) + 1][sizeof(char) + 2]) {
    return (*grid)[0][2] * 10 + (*grid)[1][1];
}

int main(void) {
    int grid[WAVE44_PLANES][WAVE44_WIDTH] = {{1, 2, 3}, {4, 5, 6}};
    printf("%d\n", wave44_grid_score(&grid));
    return 0;
}
