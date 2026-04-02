#include <stdio.h>

enum {
    A0 = 5,
    A1 = (A0 << 3) + 7,
    A2 = (A1 ^ 29) + (A0 * 4),
    A3 = ((A2 & 63) ? (A2 - 11) : (A2 + 11)),
    A4 = (A3 / 3) + (A1 % 5),
    A5 = ((A4 << 2) - (A0 + 9)),
    A6 = (A5 ^ (A3 >> 1)),
    ROWS = (A6 % 17) + 9,
    COLS = (A4 % 13) + 6
};

static int matrix[ROWS][COLS];
static int vec[(A5 % 19) + 21];

int main(void) {
    int rows = (int)(sizeof(matrix) / sizeof(matrix[0]));
    int cols = (int)(sizeof(matrix[0]) / sizeof(matrix[0][0]));
    int vlen = (int)(sizeof(vec) / sizeof(vec[0]));
    printf("%d %d %d\n", rows, cols, vlen);
    return 0;
}

