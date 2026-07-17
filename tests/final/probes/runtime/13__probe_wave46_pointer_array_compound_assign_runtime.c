#include <stdio.h>

typedef struct {
    int cells[4];
    int carry;
} Wave46Row;

static void mix_row(Wave46Row *row, int *cursor, int step) {
    int offset = (int)(cursor - row->cells);
    row->cells[offset] += step + row->carry;
    row->cells[(offset + 1) & 3] -= row->cells[offset] / 3;
    row->carry += row->cells[(offset + 2) & 3] - step;
}

static int checksum(Wave46Row row) {
    return row.carry + row.cells[0] * 3 - row.cells[1] * 5 + row.cells[2] * 7 - row.cells[3] * 11;
}

int main(void) {
    Wave46Row row = {{5, 8, 13, 21}, 3};
    int total = 0;
    int i;

    for (i = 0; i < 14; ++i) {
        int *cursor = row.cells + ((i + row.carry) & 3);
        *cursor += i;
        if ((checksum(row) ^ i) & 1) {
            mix_row(&row, cursor, i + 2);
        } else {
            row.cells[(int)(cursor - row.cells)] -= row.carry;
            row.carry += *cursor % 9;
        }
        total += checksum(row);
    }

    printf("%d %d %d %d %d\n", row.cells[0], row.cells[2], row.carry, checksum(row), total);
    return 0;
}
