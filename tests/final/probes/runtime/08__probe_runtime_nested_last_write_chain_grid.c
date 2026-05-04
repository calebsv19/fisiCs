#include <stdio.h>

struct Pair {
    int left;
    int right;
};

struct Grid {
    struct Pair rows[2][2];
    int mark;
};

static int checksum(const struct Grid *grids, int count) {
    int acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                acc = acc * 17 +
                      grids[i].rows[r][c].left * 5 +
                      grids[i].rows[r][c].right * 3 +
                      grids[i].mark;
            }
        }
    }
    return acc;
}

int main(void) {
    struct Grid grids[2] = {
        [0] = {
            .rows[0][1].left = 3,
            .rows[0] = {
                [1] = {
                    .right = 8,
                },
            },
            .rows[0][1].left = 6,
            .mark = 1,
        },
        [1] = {
            .rows[1] = {
                [0] = {
                    .left = 4,
                    .right = 2,
                },
            },
            .rows[1][0].right = 9,
            .rows[1][1].left = 7,
            .mark = 5,
        },
    };

    printf("%d %d %d %d\n",
           grids[0].rows[0][1].left,
           grids[0].rows[0][1].right,
           grids[1].rows[1][0].right,
           checksum(grids, 2));
    return 0;
}
