#include <stdio.h>

struct Pair {
    int left;
    int right;
};

struct Grid {
    struct Pair row[2];
};

int main(void) {
    struct Grid grid = {
        .row[0] = {
            .left = 1,
            .right = 2,
        },
        .row[0].right = 9,
        .row[1].left = 4,
        .row[1] = {
            .right = 5,
        },
        .row[1].left = 7,
    };

    printf(
        "%d %d %d %d\n",
        grid.row[0].left,
        grid.row[0].right,
        grid.row[1].left,
        grid.row[1].right
    );
    return 0;
}
