#include <stdio.h>

enum {
    K0 = 3,
    K1 = (K0 * 7) + (1 << 4),
    K2 = (K1 & 31) ^ 9,
    K3 = K2 ? (K2 + K0) : 0,
    K4 = (K3 << 1) - (K0 >> 1),
    ROWS = (K4 % 7) + 4,
    COLS = (K1 / 3) - 2
};

static int grid[ROWS][COLS];
static int lut[8] = {[0] = K0, [3] = K3, [7] = K4};

int main(void) {
    int rows = (int)(sizeof(grid) / sizeof(grid[0]));
    int cols = (int)(sizeof(grid[0]) / sizeof(grid[0][0]));
    int out = rows + cols + lut[0] + lut[3] + lut[7];
    printf("%d\n", out);
    return 0;
}
