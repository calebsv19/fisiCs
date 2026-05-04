#include <stdio.h>

static int alpha = 2;
static int beta = 6;
static int gamma = 9;
static int delta = 14;

static int inc1(int x) { return x + 1; }
static int inc3(int x) { return x + 3; }
static int mul2(int x) { return x * 2; }

struct Leaf {
    int base;
    int *slot;
    int (*fn)(int);
};

struct Plane {
    struct Leaf cells[2][2];
    int *aliases[2];
};

struct PlaneGrid {
    struct Plane planes[2][2];
};

static struct PlaneGrid grid = {
    .planes[0][0].cells[0][0] = { .base = 1, .slot = &alpha, .fn = inc1 },
    .planes[0][0].cells[0][0].slot = &beta,
    .planes[0][0].cells[0][1] = { .base = 4, .slot = &gamma, .fn = mul2 },
    .planes[0][0].cells[1][0] = { .base = 5, .slot = &delta, .fn = inc3 },
    .planes[0][0].aliases[0] = &alpha,
    .planes[0][0].aliases[1] = &gamma,
    .planes[0][1].cells[0][0] = { .slot = &delta, .fn = inc1 },
    .planes[0][1].cells[0][1] = { .base = 8, .slot = &beta, .fn = inc3 },
    .planes[0][1].cells[1][1] = { .base = 3, .slot = &alpha, .fn = mul2 },
    .planes[0][1].aliases[1] = &delta,
    .planes[1][0].cells[1][0] = { .base = 6, .slot = &gamma, .fn = inc1 },
    .planes[1][0].cells[1][1] = { .base = 7, .slot = &beta, .fn = mul2 },
    .planes[1][0].aliases[0] = &beta,
    .planes[1][0].aliases[1] = &alpha,
    .planes[1][1].cells[0][1] = { .base = 9, .slot = &delta, .fn = inc3 },
    .planes[1][1].cells[1][0] = { .slot = &gamma, .fn = inc1 },
    .planes[1][1].cells[1][0] = { .base = 10, .slot = &alpha, .fn = mul2 },
    .planes[1][1].aliases[0] = &delta,
    .planes[1][1].aliases[1] = &beta,
};

static unsigned checksum(void) {
    unsigned acc = 0;
    for (int p = 0; p < 2; ++p) {
        for (int q = 0; q < 2; ++q) {
            for (int r = 0; r < 2; ++r) {
                for (int c = 0; c < 2; ++c) {
                    const struct Leaf *leaf = &grid.planes[p][q].cells[r][c];
                    int slot = leaf->slot ? *leaf->slot : 0;
                    int fnv = leaf->fn ? leaf->fn(p + q + r + c + 1) : leaf->base;
                    int alias = grid.planes[p][q].aliases[c] ? *grid.planes[p][q].aliases[c] : 0;
                    acc = acc * 43u + (unsigned)(leaf->base * 13 + slot * 7 + fnv * 5 + alias);
                }
            }
        }
    }
    return acc;
}

int main(void) {
    printf("%d %d %d %u\n",
           grid.planes[0][0].cells[1][0].fn ? grid.planes[0][0].cells[1][0].fn(2) : 0,
           grid.planes[1][1].aliases[0] ? *grid.planes[1][1].aliases[0] : 0,
           grid.planes[1][1].cells[1][0].fn ? grid.planes[1][1].cells[1][0].fn(3) : 0,
           checksum());
    return 0;
}
