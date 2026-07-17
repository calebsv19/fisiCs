#include <stddef.h>
#include <stdio.h>

struct Matrix {
    unsigned short rows[3][3];
};

static int row_score(unsigned short (*row)[3], int offset) {
    void *opaque = (void *)row;
    unsigned short (*roundtrip)[3] = (unsigned short (*)[3])opaque;
    unsigned int total = 0u;
    int i;
    for (i = 0; i < 3; i++) {
        total += (unsigned int)(unsigned short)((*roundtrip)[i] + (unsigned short)(offset + i));
    }
    return (int)total;
}

int main(void) {
    struct Matrix matrices[2] = {
        {{{1u, 2u, 3u}, {4u, 5u, 6u}, {7u, 8u, 9u}}},
        {{{21u, 22u, 23u}, {31u, 32u, 33u}, {41u, 42u, 43u}}}
    };

    unsigned short (*base)[3] = matrices[1].rows;
    unsigned short (*selected)[3] = 1 ? &base[2] : &matrices[0].rows[0];
    unsigned short (*previous)[3] = (unsigned short (*)[3])(void *)(selected - 1);
    ptrdiff_t row_delta = selected - matrices[1].rows;
    ptrdiff_t cell_delta = &(*selected)[2] - &(*selected)[0];
    int contiguous = (&previous[0][3] == &previous[1][0]);

    printf("%d %ld %ld %d %u\n",
           row_score(previous, 5),
           (long)row_delta,
           (long)cell_delta,
           contiguous,
           (unsigned int)(unsigned short)((*selected)[2] + matrices[0].rows[2][1]));
    return 0;
}
