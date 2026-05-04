#include <stdio.h>

static int alpha = 5;
static int beta = 9;

static int add_three(int x) {
    return x + 3;
}

static int mul_two(int x) {
    return x * 2;
}

struct Cell {
    int *ptr;
    int (*fn)(int);
};

struct Table {
    struct Cell cells[3];
    int *alias;
};

static struct Table table = {
    .cells[0].ptr = &alpha,
    .cells[1].fn = add_three,
    .cells[2] = {
        .ptr = &beta,
        .fn = mul_two,
    },
    .alias = &alpha,
};

int main(void) {
    int v0 = table.cells[0].ptr ? *table.cells[0].ptr : 0;
    int v1 = table.cells[1].fn ? table.cells[1].fn(4) : 0;
    int v2 = table.cells[2].ptr ? *table.cells[2].ptr : 0;
    int v3 = table.cells[2].fn ? table.cells[2].fn(7) : 0;
    int v4 = table.alias ? *table.alias : 0;

    printf("%d %d %d %d %d\n", v0, v1, v2, v3, v4);
    return 0;
}
