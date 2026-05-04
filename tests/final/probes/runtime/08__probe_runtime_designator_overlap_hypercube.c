#include <stdio.h>

struct Cell {
    int left;
    int right;
    int mark;
};

struct Grid {
    struct Cell cells[2][2][2];
    int tail;
};

static int checksum(const struct Grid *grids, int count) {
    int acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Cell *cell = &grids[i].cells[x][y][z];
                    acc = acc * 41 +
                          cell->left * 7 +
                          cell->right * 5 +
                          cell->mark * 3 +
                          grids[i].tail;
                }
            }
        }
    }

    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .cells[0][1] = {
                [0] = {
                    .left = 2,
                    .right = 4,
                },
            },
            .cells[0][1][0].left = 7,
            .cells[1][0][1] = {
                .mark = 3,
            },
            .cells[1][0][1].right = 8,
            .tail = 1,
        },
        [1] = {
            .cells[1][1] = {
                [1] = {
                    .left = 6,
                    .right = 5,
                },
            },
            .cells[1][1][1].right = 9,
            .cells[1][1][0] = {
                .left = 4,
                .mark = 7,
            },
            .cells[1][1][0].mark = 10,
            .cells[0][0][1] = {
                .right = 6,
            },
            .tail = 3,
        },
    };

    printf("%d %d %d %d\n",
           grids[0].cells[0][1][0].left,
           grids[0].cells[1][0][1].right,
           grids[1].cells[1][1][1].right,
           checksum(grids, 2));
    return 0;
}
