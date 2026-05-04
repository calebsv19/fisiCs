#include <stdio.h>

struct Pair {
    int x;
    int y;
};

static int checksum(const struct Pair (*rows)[2], int count) {
    int acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < 2; ++j) {
            acc = acc * 13 + rows[i][j].x * 5 + rows[i][j].y;
        }
    }
    return acc;
}

int main(void) {
    struct Pair rows[][2] = {
        [2] = {
            [1] = {
                .y = 4,
            },
        },
        [4] = {
            [0] = {
                .x = 3,
                .y = 5,
            },
        },
    };
    size_t count = sizeof(rows) / sizeof(rows[0]);

    printf("%zu %d %d %d\n", count, rows[2][1].y, rows[4][0].x, checksum(rows, (int)count));
    return 0;
}
