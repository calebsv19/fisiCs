#include <stdio.h>

int abi_vla_reduce(int rows, int cols, int matrix[rows][cols]);

int main(void) {
    int matrix[3][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12}
    };
    int value = abi_vla_reduce(3, 4, matrix);
    printf("%d\n", value);
    return 0;
}

