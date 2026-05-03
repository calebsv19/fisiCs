#include <stdio.h>

struct Cell {
    int x;
    int y;
};

struct Grid {
    struct Cell cells[2][2];
    int tail;
};

static struct Grid build_grid(void) {
    struct Grid grid = {
        .cells = {1, 0, 2, 3, 4, 5, 6, 0},
        .tail = 11,
    };
    return grid;
}

int main(void) {
    struct Grid grid = build_grid();
    printf("%d %d %d %d %d %d %d %d %d\n",
           grid.cells[0][0].x,
           grid.cells[0][0].y,
           grid.cells[0][1].x,
           grid.cells[0][1].y,
           grid.cells[1][0].x,
           grid.cells[1][0].y,
           grid.cells[1][1].x,
           grid.cells[1][1].y,
           grid.tail);
    return 0;
}
