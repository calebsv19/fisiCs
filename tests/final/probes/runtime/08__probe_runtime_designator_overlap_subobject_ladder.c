#include <stdio.h>

union Mix {
    int value;
    struct {
        short lo;
        short hi;
    } pair;
};

struct Cell {
    union Mix mix;
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
                    acc = acc * 37 +
                          cell->mix.pair.lo * 7 +
                          cell->mix.pair.hi * 5 +
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
                [0] = { .mix.pair = { 2, 4 }, .mark = 1 },
            },
            .cells[0][1][0].mix.value = 99,
            .cells[0][1][0].mark = 7,
            .cells[1][0][1] = { .mix.pair = { 3, 8 }, .mark = 5 },
            .tail = 2,
        },
        [1] = {
            .cells[1][1] = {
                [1] = { .mix.pair = { 6, 5 }, .mark = 4 },
            },
            .cells[1][1][1].mix.value = 1234,
            .cells[1][1][1].mark = 9,
            .cells[1][1][0] = { .mix.pair = { 4, 7 }, .mark = 10 },
            .tail = 3,
        },
    };

    printf("%d %d %d %d\n",
           grids[0].cells[0][1][0].mix.pair.lo,
           grids[0].cells[1][0][1].mix.pair.hi,
           grids[1].cells[1][1][1].mix.pair.lo,
           checksum(grids, 2));
    return 0;
}
