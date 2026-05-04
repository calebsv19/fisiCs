#include <stdio.h>

union Mix {
    int value;
    struct {
        short lo;
        short hi;
    } pair;
};

struct Slot {
    union Mix mix;
    int mark;
};

struct Plane {
    struct Slot rows[2][2];
    int tag;
};

struct PlaneGrid {
    struct Plane planes[2][2];
};

static int checksum(const struct Plane planes[2][2]) {
    int acc = 0;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int r = 0; r < 2; ++r) {
                for (int c = 0; c < 2; ++c) {
                    const struct Slot *slot = &planes[x][y].rows[r][c];
                    acc = acc * 31 +
                          slot->mix.pair.lo * 11 +
                          slot->mix.pair.hi * 7 +
                          slot->mark * 3 +
                          planes[x][y].tag;
                }
            }
        }
    }
    return acc;
}

int main(void) {
    struct PlaneGrid grid = {
        .planes[0][0].rows[0][0].mix.pair = { 1, 4 },
        .planes[0][0].rows[0][0].mark = 2,
        .planes[0][0].rows[0][0].mix.value = 99,
        .planes[0][0].rows[1][1].mix.pair = { 3, 6 },
        .planes[0][0].rows[1][1].mark = 5,
        .planes[0][0].tag = 2,
        .planes[0][1].rows[1][0].mix.pair = { 7, 8 },
        .planes[0][1].rows[1][0].mark = 4,
        .planes[0][1].rows[1][0].mix.value = 511,
        .planes[0][1].tag = 3,
        .planes[1][0].rows[0][1].mix.pair = { 2, 9 },
        .planes[1][0].rows[0][1].mark = 6,
        .planes[1][0].tag = 5,
        .planes[1][1].rows[1][1].mix.pair = { 5, 7 },
        .planes[1][1].rows[1][1].mark = 9,
        .planes[1][1].rows[1][1].mix.value = 777,
        .planes[1][1].tag = 7,
    };

    printf("%d %d %d %d\n",
           grid.planes[0][0].rows[0][0].mix.pair.lo,
           grid.planes[0][1].rows[1][0].mix.pair.lo,
           grid.planes[1][1].rows[1][1].mix.pair.lo,
           checksum(grid.planes));
    return 0;
}
