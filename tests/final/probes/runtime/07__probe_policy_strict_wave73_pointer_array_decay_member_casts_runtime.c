#include <stddef.h>
#include <stdio.h>

struct Grid {
    unsigned char cells[2][3];
};

static int row_sum(unsigned char (*row)[3]) {
    unsigned char *decayed = *row;
    return (int)decayed[0] + (int)decayed[1] + (int)decayed[2];
}

int main(void) {
    struct Grid grids[2] = {
        {{{1u, 2u, 3u}, {4u, 5u, 6u}}},
        {{{7u, 8u, 9u}, {10u, 11u, 12u}}}
    };

    unsigned char (*selected)[3] = 1 ? &grids[1].cells[0] : &grids[0].cells[0];
    void *opaque = (void *)selected;
    unsigned char (*roundtrip)[3] = (unsigned char (*)[3])opaque;
    unsigned char *flat = *roundtrip;
    ptrdiff_t row_delta = roundtrip - &grids[0].cells[0];
    ptrdiff_t cell_delta = &flat[2] - &flat[0];
    int next_row = (&roundtrip[0][3] == &roundtrip[1][0]);

    printf("%d %ld %ld %d %u\n",
           row_sum(roundtrip),
           (long)row_delta,
           (long)cell_delta,
           next_row,
           (unsigned int)(unsigned char)(flat[2] + grids[0].cells[1][2]));
    return 0;
}
