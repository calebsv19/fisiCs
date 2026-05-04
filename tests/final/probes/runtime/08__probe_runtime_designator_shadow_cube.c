#include <stdio.h>

struct Cell {
    int left;
    int right;
    int mark;
    int flag;
};

struct Grid {
    struct Cell plane[2][2][2];
    int tag;
};

static int checksum(const struct Grid *grids, int count) {
    int acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Cell *cell = &grids[i].plane[x][y][z];
                    acc = acc * 17 +
                          cell->left * 11 +
                          cell->right * 7 +
                          cell->mark * 5 +
                          cell->flag * 3 +
                          grids[i].tag;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .plane[0][1] = {
                [0] = {
                    .left = 2,
                    .right = 4,
                },
            },
            .plane[0][1][0].left = 7,
            .plane[1][0][1] = {
                .mark = 3,
                .flag = 5,
            },
            .plane[1][0][1].flag = 8,
            .tag = 1,
        },
        [1] = {
            .plane[1][1] = {
                [1] = {
                    .left = 6,
                    .right = 5,
                },
            },
            .plane[1][1][1].right = 9,
            .plane[1][1][0] = {
                .left = 4,
                .mark = 7,
            },
            .plane[1][1][0].mark = 10,
            .plane[0][0][1].flag = 6,
            .tag = 3,
        },
    };

    printf("%d %d %d %d\n",
           grids[0].plane[0][1][0].left,
           grids[0].plane[1][0][1].flag,
           grids[1].plane[1][1][1].right,
           checksum(grids, 2));
    return 0;
}
