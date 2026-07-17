#include <stdio.h>

enum Axis {
    AXIS_NEG = -3,
    AXIS_ZERO = 0,
    AXIS_TWO = 2,
    AXIS_FIVE = 5
};

struct Cell {
    enum Axis axis;
    unsigned char lane;
    signed char adjust;
};

static int fold_cells(struct Cell *cells, int count) {
    static int weights[3][4] = {
        {3, 5, 7, 11},
        {13, 17, 19, 23},
        {29, 31, 37, 41}
    };
    int sum = 0;
    int i;
    for (i = 0; i < count; ++i) {
        int axis = (int)cells[i].axis;
        int row = (axis < 0 ? -axis : axis) % 3;
        int col = ((int)(unsigned char)(cells[i].lane + (unsigned char)cells[i].axis) + i) & 3;
        int promoted = (int)(unsigned char)(cells[i].lane + (unsigned char)cells[i].adjust);
        sum += weights[row][col] + promoted - axis;
    }
    return sum;
}

int main(void) {
    struct Cell cells[5] = {
        {AXIS_NEG, 250u, -7},
        {AXIS_ZERO, 19u, 6},
        {AXIS_TWO, 201u, -9},
        {AXIS_FIVE, 44u, 13},
        {AXIS_NEG, 128u, 27}
    };
    int total = fold_cells(cells, 5);
    printf("%d %d %d\n", total, total & 255, total % 17);
    return 0;
}
