#include <stddef.h>
#include <stdio.h>

static int row_total(int (*row)[4]) {
    int *cells = *row;
    return cells[0] + cells[1] + cells[2] + cells[3];
}

int main(void) {
    int grid[3][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12}
    };

    int (*selected)[4] = 1 ? &grid[2] : &grid[0];
    int *decayed = *selected;
    void *opaque = (void *)selected;
    int (*roundtrip)[4] = (int (*)[4])opaque;
    ptrdiff_t row_delta = roundtrip - &grid[0];
    ptrdiff_t cell_delta = (&(*roundtrip)[4]) - (&(*roundtrip)[1]);
    int adjacent_rows = (&grid[1][4] == &grid[2][0]);

    printf("%d %ld %ld %d %d\n",
           row_total(roundtrip),
           (long)row_delta,
           (long)cell_delta,
           adjacent_rows,
           decayed[2]);
    return 0;
}
