#include <stdio.h>

struct Cell {
    int left;
    int right;
    int mark;
    int flag;
};

struct Cube {
    struct Cell plane[2][2][2];
    int tail;
};

static int checksum(const struct Cube *cubes, int count) {
    int acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Cell *cell = &cubes[i].plane[x][y][z];
                    acc = acc * 23 +
                          cell->left * 11 +
                          cell->right * 7 +
                          cell->mark * 5 +
                          cell->flag * 3 +
                          cubes[i].tail;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct Cube cubes[2] = {
        [0] = {
            .plane[0][1] = {
                [1] = {
                    .left = 3,
                    .right = 4,
                },
            },
            .plane[0][1][1].left = 8,
            .plane[1][0][0] = {
                .mark = 2,
                .flag = 6,
            },
            .plane[1][0][0].flag = 9,
            .tail = 1,
        },
        [1] = {
            .plane[1][1] = {
                [0] = {
                    .left = 6,
                    .right = 5,
                },
            },
            .plane[1][1][0].right = 9,
            .plane[1][1][1] = {
                .left = 4,
                .mark = 7,
            },
            .plane[1][1][1].mark = 10,
            .plane[0][0][1].flag = 5,
            .tail = 3,
        },
    };

    printf("%d %d %d %d\n",
           cubes[0].plane[0][1][1].left,
           cubes[0].plane[1][0][0].flag,
           cubes[1].plane[1][1][0].right,
           checksum(cubes, 2));
    return 0;
}
