#include <stdio.h>

struct Cell {
    int left;
    int right;
    int mark;
};

struct Cube {
    struct Cell plane[2][2];
    int tag;
};

static int checksum(const struct Cube *cubes, int count) {
    int acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Cell *cell = &cubes[i].plane[r][c];
                acc = acc * 19 +
                      cell->left * 11 +
                      cell->right * 7 +
                      cell->mark * 5 +
                      cubes[i].tag;
            }
        }
    }
    return acc;
}

int main(void) {
    struct Cube cubes[2] = {
        [0] = {
            .plane[0][1].left = 3,
            .plane[0] = {
                [1] = {
                    .left = 8,
                    .right = 4,
                },
            },
            .plane[1][0].mark = 2,
            .tag = 1,
        },
        [1] = {
            .plane[1][1].right = 5,
            .plane[1] = {
                [1] = {
                    .left = 6,
                    .right = 9,
                },
            },
            .plane[0][0].mark = 7,
            .tag = 3,
        },
    };

    printf("%d %d %d %d\n",
           cubes[0].plane[0][1].left,
           cubes[0].plane[1][0].mark,
           cubes[1].plane[1][1].right,
           checksum(cubes, 2));
    return 0;
}
