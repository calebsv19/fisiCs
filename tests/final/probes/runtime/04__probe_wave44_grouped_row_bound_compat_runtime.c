#include <stdio.h>

enum { WAVE44_COLUMNS = 3 };

int wave44_row_sum(int (*rows)[WAVE44_COLUMNS], int count);

int wave44_row_sum(int rows[][sizeof(char) + 2], int count) {
    int total = 0;
    int row;
    int column;

    for (row = 0; row < count; ++row) {
        for (column = 0; column < WAVE44_COLUMNS; ++column) {
            total += rows[row][column];
        }
    }
    return total;
}

int main(void) {
    int rows[2][WAVE44_COLUMNS] = {{1, 2, 3}, {4, 5, 6}};
    printf("%d\n", wave44_row_sum(rows, 2));
    return 0;
}
