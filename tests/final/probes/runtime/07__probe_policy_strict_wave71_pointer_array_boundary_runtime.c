#include <stddef.h>
#include <stdio.h>

static int row_sum(int (*row)[3]) {
    int *base = *row;
    return base[0] + base[1] + base[2];
}

int main(void) {
    int grid[2][3] = {
        {1, 3, 5},
        {2, 4, 8}
    };

    int (*chosen)[3] = 1 ? &grid[1] : &grid[0];
    int *decayed = *chosen;
    void *opaque = chosen;
    int (*roundtrip)[3] = (int (*)[3])opaque;
    ptrdiff_t row_delta = roundtrip - &grid[0];
    ptrdiff_t cell_delta = (&(*roundtrip)[3]) - &(*roundtrip)[0];
    int one_past_order = (&grid[0][3] == &grid[1][0]);

    printf("%d %ld %ld %d %d\n",
           row_sum(roundtrip),
           (long)row_delta,
           (long)cell_delta,
           one_past_order,
           decayed[2]);
    return 0;
}
