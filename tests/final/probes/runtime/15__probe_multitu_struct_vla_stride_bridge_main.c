#include <stdio.h>

struct Lane {
    int base;
    int stride;
};

int mt13_vla_fold(int rows, int cols, int grid[rows][cols], struct Lane lane);
int mt13_vla_edge(int rows, int cols, int grid[rows][cols]);

int main(void) {
    enum { ROWS = 5, COLS = 7 };
    int grid[ROWS][COLS];
    struct Lane lane = {5, 3};

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            grid[r][c] = ((r + 1) * (c + 3) + r * c + 7) % 97;
        }
    }

    printf("%d %d\n", mt13_vla_fold(ROWS, COLS, grid, lane), mt13_vla_edge(ROWS, COLS, grid));
    return 0;
}
