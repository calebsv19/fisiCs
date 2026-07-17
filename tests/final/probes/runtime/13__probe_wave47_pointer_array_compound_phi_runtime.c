#include <stdio.h>

typedef struct {
    int cells[5];
    int carry;
} Wave47Row;

static int checksum(Wave47Row row) {
    return row.carry
        + row.cells[0] * 2
        - row.cells[1] * 3
        + row.cells[2] * 5
        - row.cells[3] * 7
        + row.cells[4] * 11;
}

static void apply_slot(Wave47Row *row, int **slots, int pick, int step) {
    int *selected = slots[pick % 5];
    int index = (int)(selected - row->cells);
    *selected += row->carry + step;
    row->cells[(index + 2) % 5] -= *selected / 4;
    row->carry += row->cells[(index + 3) % 5] - index;
}

int main(void) {
    Wave47Row row = {{6, 10, 15, 21, 28}, 4};
    int *slots[5];
    int total = 0;
    int i;

    for (i = 0; i < 5; ++i) {
        slots[i] = row.cells + ((i * 2 + 1) % 5);
    }

    for (i = 0; i < 13; ++i) {
        int pick = (checksum(row) + i) % 5;
        if (pick < 0) {
            pick = -pick;
        }
        if ((i ^ row.carry) & 1) {
            *slots[pick] += i * 2 - row.carry;
            row.carry += (int)(slots[pick] - row.cells);
        } else {
            apply_slot(&row, slots, pick + 2, i + 3);
        }
        slots[(pick + 1) % 5] = row.cells + ((pick + row.carry + 10) % 5);
        total += checksum(row);
    }

    printf("%d %d %d %d %d\n", row.cells[1], row.cells[3], row.carry, checksum(row), total);
    return 0;
}
