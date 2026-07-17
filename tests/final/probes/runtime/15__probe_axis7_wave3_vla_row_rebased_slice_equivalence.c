#include <stdio.h>

static unsigned int row_checksum(unsigned int rows, unsigned int cols,
                                 unsigned int matrix[rows][cols]) {
    unsigned int total = 0u;
    unsigned int row;
    unsigned int col;

    for (row = 0u; row < rows; ++row) {
        for (col = 0u; col < cols; ++col) {
            total += matrix[row][col] * (row * 11u + col * 3u + 1u);
        }
    }
    return total;
}

static unsigned int rebased_checksum(unsigned int rows, unsigned int cols,
                                     unsigned int matrix[rows][cols]) {
    unsigned int total = 0u;
    unsigned int *base = &matrix[0][0];
    unsigned int row;
    unsigned int col;

    for (row = 0u; row < rows; ++row) {
        unsigned int *slice = base + row * cols;
        for (col = 0u; col < cols; ++col) {
            total += slice[col] * (row * 11u + col * 3u + 1u);
        }
    }
    return total;
}

int main(void) {
    unsigned int rows = 3u;
    unsigned int cols = 4u;
    unsigned int matrix[3][4] = {
        {2u, 5u, 11u, 17u},
        {23u, 29u, 31u, 37u},
        {41u, 43u, 47u, 53u}
    };
    unsigned int a = row_checksum(rows, cols, matrix);
    unsigned int b = rebased_checksum(rows, cols, matrix);

    printf("%u %u %u\n", a, b, a == b);
    return 0;
}
