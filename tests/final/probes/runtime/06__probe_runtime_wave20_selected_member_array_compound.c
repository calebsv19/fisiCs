#include <stdio.h>

typedef struct {
    int value;
    int lanes[3];
} Cell;

typedef struct {
    Cell cells[2];
    volatile int stamp;
} Grid;

int main(void) {
    Grid grids[2] = {
        {{{4, {6, 8, 10}}, {12, {14, 16, 18}}}, 20},
        {{{22, {24, 26, 28}}, {30, {32, 34, 36}}}, 38},
    };

    int pick = grids[1].cells[0].lanes[2] > grids[0].cells[1].lanes[1];
    Grid *selected = pick ? &grids[1] : &grids[0];
    Cell *cell = &selected->cells[pick ? 0 : 1];

    cell->lanes[pick ? 2 : 0] += selected->stamp;
    ++cell->value;
    selected->stamp += cell->lanes[pick ? 2 : 0];

    int total = cell->value + cell->lanes[pick ? 2 : 0] + selected->stamp;
    printf("%d %d %d\n", cell->value, cell->lanes[pick ? 2 : 0], total);
    return 0;
}
