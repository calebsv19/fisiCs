#include <stdio.h>

typedef union {
    struct {
        int x;
        int y;
        int z;
    } xyz;
    int raw[3];
} Wave45Cell;

typedef struct {
    int mode;
    Wave45Cell cells[2];
    int bias;
} Wave45Row;

static int cell_score(Wave45Cell cell, int mode) {
    if (mode == 1) {
        return cell.xyz.x * 2 + cell.xyz.y * 3 - cell.xyz.z;
    }
    return cell.raw[0] - cell.raw[1] * 2 + cell.raw[2] * 5;
}

static int row_score(Wave45Row row) {
    return row.mode * 17 + row.bias + cell_score(row.cells[0], row.mode) - cell_score(row.cells[1], row.mode);
}

int main(void) {
    Wave45Row row = {1, {{{2, 5, 7}}, {{3, 6, 8}}}, 11};
    int total = 0;
    int i;

    for (i = 0; i < 9; ++i) {
        Wave45Row next = row;
        if ((row_score(row) + i) & 1) {
            next = (Wave45Row){2, {{{i + 4, i * 2 + 1, row.bias - i}}, {{row.mode + i, row.bias % 7, i + 9}}}, row.bias + i * 3};
        } else {
            next.cells[i & 1] = (Wave45Cell){{row.bias + i, row.mode + total % 5, row.cells[0].raw[2] - i}};
            next.bias -= i + row.mode;
        }

        if ((row_score(next) > row_score(row)) && (next.cells[i & 1].raw[0] >= row.mode)) {
            row = next;
        } else {
            row.cells[(i + 1) & 1].raw[2] += i + 1;
        }

        total += row_score(row);
    }

    printf("%d %d %d %d %d\n", row.mode, row.bias, row.cells[0].raw[0], row_score(row), total);
    return 0;
}
