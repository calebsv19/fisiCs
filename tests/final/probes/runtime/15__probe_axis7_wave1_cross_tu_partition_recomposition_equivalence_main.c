#include <stdio.h>

typedef struct Cell { unsigned int lane, payload; } Cell;

Cell transform_cell(Cell in);
unsigned int fold_cells(const Cell *cells, unsigned int count);

int main(void) {
    const Cell source[] = {{0u, 5u}, {1u, 8u}, {2u, 13u}, {3u, 21u}};
    Cell direct[4];
    Cell staged[4];
    for (unsigned int i = 0u; i < 4u; ++i) direct[i] = transform_cell(source[i]);
    for (unsigned int i = 0u; i < 2u; ++i) staged[i] = transform_cell(source[i]);
    for (unsigned int i = 2u; i < 4u; ++i) staged[i] = transform_cell(source[i]);
    {
        unsigned int a = fold_cells(direct, 4u);
        unsigned int b = fold_cells(staged, 4u);
        printf("%u %u %u\n", a, b, a == b);
    }
    return 0;
}
