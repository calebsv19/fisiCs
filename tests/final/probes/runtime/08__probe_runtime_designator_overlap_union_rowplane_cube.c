#include <stdio.h>

union Pair {
    int value;
    struct {
        short lo;
        short hi;
    } halves;
};

struct Entry {
    union Pair pair;
    int flag;
};

struct Cube {
    struct Entry rows[2][2][2];
    int tail;
};

static int checksum(const struct Cube *cubes, int count) {
    int acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const struct Entry *entry = &cubes[i].rows[x][y][z];
                    acc = acc * 39 +
                          entry->pair.halves.lo * 7 +
                          entry->pair.halves.hi * 5 +
                          entry->flag * 3 +
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
            .rows[0] = {
                [0] = {
                    [1] = {
                        .pair.halves = { 2, 6 },
                        .flag = 1,
                    },
                },
                [1] = {
                    [0] = {
                        .pair.halves = { 4, 8 },
                        .flag = 3,
                    },
                },
            },
            .rows[0][1][0].pair.value = 777,
            .tail = 4,
        },
        [1] = {
            .rows[1] = {
                [0] = {
                    [0] = {
                        .pair.halves = { 3, 5 },
                        .flag = 7,
                    },
                },
                [1] = {
                    [1] = {
                        .pair.halves = { 6, 9 },
                        .flag = 2,
                    },
                },
            },
            .rows[1][1][1].pair.value = 1444,
            .rows[1][0][0].flag = 11,
            .tail = 6,
        },
    };

    printf("%d %d %d %d\n",
           cubes[0].rows[0][1][0].pair.halves.lo,
           cubes[0].rows[0][0][1].pair.halves.hi,
           cubes[1].rows[1][1][1].pair.halves.lo,
           checksum(cubes, 2));
    return 0;
}
