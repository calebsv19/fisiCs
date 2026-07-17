#include <stdio.h>

typedef struct {
    volatile int sensor;
    int payload[2];
} Cell;

static int read_const_cell(const Cell *cell, int pick) {
    const int *selected = pick ? &cell->payload[1] : &cell->payload[0];
    return *selected + cell->sensor;
}

int main(void) {
    Cell cells[2] = {
        {3, {5, 7}},
        {11, {13, 17}},
    };

    int flag = cells[0].payload[1] < cells[1].payload[0];
    volatile int *sensor = flag ? &cells[0].sensor : &cells[1].sensor;
    *sensor += 19;

    int *payload = flag ? &cells[1].payload[0] : &cells[0].payload[1];
    *payload += read_const_cell(flag ? &cells[0] : &cells[1], flag);

    const Cell *chosen = flag ? (const Cell *)&cells[1] : (const Cell *)&cells[0];
    const int *preserved = flag ? &chosen->payload[0] : &chosen->payload[1];

    printf("%d %d %d %d\n",
           cells[0].sensor,
           cells[1].payload[0],
           *preserved,
           read_const_cell(chosen, flag));
    return 0;
}
