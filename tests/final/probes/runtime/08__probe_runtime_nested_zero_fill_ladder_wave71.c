#include <stdio.h>

struct Pair {
    int x;
    int y;
};

struct Cell {
    struct Pair pairs[2];
    int mark;
};

struct Grid {
    struct Cell cells[2][2];
    int tail;
};

static int checksum(const struct Grid grids[2]) {
    int acc = 0;

    for (int g = 0; g < 2; ++g) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Cell *cell = &grids[g].cells[r][c];
                acc = acc * 29 +
                      cell->pairs[0].x * 13 +
                      cell->pairs[0].y * 11 +
                      cell->pairs[1].x * 7 +
                      cell->pairs[1].y * 5 +
                      cell->mark * 3 +
                      grids[g].tail;
            }
        }
    }

    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .cells[0][1].pairs[1].y = 7,
            .cells[0][1].mark = 2,
            .cells[1][0].pairs[0].x = 3,
            .cells[1][0].pairs[1].x = 4,
            .cells[1][0].pairs[1].y = 5,
            .cells[1][0].mark = 6,
            .tail = 1,
        },
        [1] = {
            .cells[1][1].pairs[0].y = 9,
            .cells[1][1].pairs[1].x = 8,
            .cells[1][1].mark = 4,
            .cells[0][0].pairs[0].x = 2,
            .tail = 3,
        },
    };

    printf("%d %d %d %d\n",
           grids[0].cells[0][1].pairs[0].x,
           grids[0].cells[0][1].pairs[1].y,
           grids[1].cells[1][1].pairs[0].x,
           checksum(grids));
    return 0;
}
