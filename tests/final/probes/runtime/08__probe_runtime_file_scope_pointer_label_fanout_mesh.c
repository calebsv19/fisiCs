#include <stdio.h>

static int alpha = 3;
static int beta = 5;
static int gamma = 7;

static int add_four(int x) {
    return x + 4;
}

static int mul_five(int x) {
    return x * 5;
}

struct Leaf {
    int *slot;
    int (*fn)(int);
    const char *label;
};

struct Grid {
    struct Leaf cells[2][2];
    int *mirrors[2];
};

static struct Grid grids[2] = {
    [0] = {
        .cells[0][0].slot = &alpha,
        .cells[0][0].fn = add_four,
        .cells[0][0].label = "ab",
        .cells[0][1].label = "xy",
        .mirrors[1] = &gamma,
    },
    [1] = {
        .cells[1][0] = {
            .slot = &beta,
            .fn = mul_five,
            .label = "mn",
        },
        .cells[1][1].slot = &gamma,
        .cells[1][1].fn = add_four,
        .cells[1][1].label = "qr",
        .mirrors[0] = &alpha,
        .mirrors[1] = &beta,
    },
};

static unsigned checksum(void) {
    unsigned acc = 0;

    for (int i = 0; i < 2; ++i) {
        for (int r = 0; r < 2; ++r) {
            for (int c = 0; c < 2; ++c) {
                const struct Leaf *leaf = &grids[i].cells[r][c];
                int slot = leaf->slot ? *leaf->slot : 0;
                int fnv = leaf->fn ? leaf->fn(r + c + 2) : 0;
                int text = leaf->label ? leaf->label[0] + leaf->label[1] : 0;
                int mirror = grids[i].mirrors[c] ? *grids[i].mirrors[c] : 0;
                acc = acc * 41u + (unsigned)(slot * 7 + fnv * 5 + text + mirror);
            }
        }
    }
    return acc;
}

int main(void) {
    printf("%d %d %c %u\n",
           grids[0].cells[0][0].fn ? grids[0].cells[0][0].fn(2) : 0,
           grids[1].mirrors[1] ? *grids[1].mirrors[1] : 0,
           grids[1].cells[1][1].label ? grids[1].cells[1][1].label[1] : '?',
           checksum());
    return 0;
}
