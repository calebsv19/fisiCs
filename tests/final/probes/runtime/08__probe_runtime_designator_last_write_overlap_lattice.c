#include <stdio.h>

struct Cell {
    int x;
    int y;
};

struct Block {
    struct Cell cells[3];
    int tag;
};

int main(void) {
    struct Block blocks[2] = {
        [0] = {
            .cells[1].x = 4,
            .cells[1] = {
                .y = 8,
            },
            .cells[1].x = 6,
            .tag = 1,
        },
        [1] = {
            .cells[0] = {
                .x = 2,
                .y = 3,
            },
            .cells[0].y = 9,
            .cells[2].x = 5,
            .cells[2] = {
                .y = 7,
            },
            .cells[2].x = 11,
        },
    };

    printf(
        "%d %d %d %d %d\n",
        blocks[0].cells[1].x,
        blocks[0].cells[1].y,
        blocks[1].cells[0].x,
        blocks[1].cells[0].y,
        blocks[1].cells[2].x + blocks[1].cells[2].y
    );
    return 0;
}
