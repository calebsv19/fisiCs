#include <stdio.h>

typedef struct {
    int cells[6];
    int bias;
} Wave48Row;

static int checksum(Wave48Row row) {
    return row.bias
        + row.cells[0] * 2
        - row.cells[1] * 3
        + row.cells[2] * 5
        - row.cells[3] * 7
        + row.cells[4] * 11
        - row.cells[5] * 13;
}

int main(void) {
    Wave48Row row = {{4, 9, 15, 22, 30, 39}, 6};
    int *slots[3];
    int total = 0;
    int i;

    slots[0] = row.cells + 1;
    slots[1] = row.cells + 3;
    slots[2] = row.cells + 5;

    for (i = 0; i < 12; ++i) {
        int pick = (checksum(row) + i + 6000) % 3;
        int *cursor = ((i ^ row.bias) & 1) ? slots[pick] : row.cells + ((pick + i + 2) % 6);
        int offset = (int)(cursor - row.cells);
        *cursor += row.bias - i + offset;
        row = (offset & 1)
            ? (Wave48Row){{row.cells[5], row.cells[0] + i, row.cells[1] - offset, row.cells[2], row.cells[3] + row.bias, row.cells[4] - i}, row.bias + offset}
            : (Wave48Row){{row.cells[1] + offset, row.cells[2], row.cells[3] - i, row.cells[4], row.cells[5] + row.bias, row.cells[0]}, row.bias - i};
        slots[pick] = row.cells + ((offset + row.bias + 6000) % 6);
        total += checksum(row);
    }

    printf("%d %d %d %d %d\n", row.cells[0], row.cells[4], row.bias, checksum(row), total);
    return 0;
}
