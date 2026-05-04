#include <stdio.h>

struct Node {
    int value;
    int *value_ptr;
    int (*adjust)(int);
};

struct Grid {
    struct Node cells[2][2];
    int *tag_ptr;
};

static int plus_one(int x) { return x + 1; }
static int plus_three(int x) { return x + 3; }

static int base_a = 4;
static int base_b = 9;
static int tag_value = 6;

static int checksum(const struct Grid *grids, int count) {
    int acc = 0;

    for (int i = 0; i < count; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Node *cell = &grids[i].cells[r][c];
                int pointed = cell->value_ptr ? *cell->value_ptr : 0;
                int adjusted = cell->adjust ? cell->adjust(cell->value) : cell->value;
                acc = acc * 29 + adjusted * 7 + pointed * 5;
            }
        }
        acc += grids[i].tag_ptr ? *grids[i].tag_ptr : 0;
    }

    return acc;
}

int main(void) {
    static struct Grid grids[2] = {
        [0] = {
            .cells[0][0].value = 1,
            .cells[0][0] = {
                .value_ptr = &base_a,
                .adjust = plus_one,
            },
            .cells[1][1] = {
                .value = 5,
                .value_ptr = &base_b,
                .adjust = plus_three,
            },
            .tag_ptr = &tag_value,
        },
        [1] = {
            .cells[0] = {
                [1] = {
                    .value = 7,
                    .value_ptr = &base_b,
                    .adjust = plus_one,
                },
            },
            .cells[0] = {
                [1] = {
                    .value = 8,
                    .value_ptr = &base_a,
                    .adjust = plus_three,
                },
            },
            .cells[1][0].value = 2,
            .tag_ptr = &base_b,
        },
    };

    printf("%d %d %d %d\n",
           grids[0].cells[0][0].adjust(grids[0].cells[0][0].value),
           *grids[0].cells[1][1].value_ptr,
           grids[1].cells[0][1].adjust(grids[1].cells[0][1].value),
           checksum(grids, 2));
    return 0;
}
