#include <stdio.h>

typedef struct Cell46B {
    int left;
    int right;
} Cell46B;

typedef struct Matrix46B {
    Cell46B cells[2];
    int marker;
} Matrix46B;

typedef Matrix46B (*MatrixMaker46B)(Cell46B, int);

MatrixMaker46B wave46_select_matrix_maker(int route);

int main(void) {
    Cell46B seed = {6, 8};
    MatrixMaker46B maker = wave46_select_matrix_maker(3);
    Matrix46B matrix = maker(seed, 4);
    int checksum = matrix.cells[0].left + matrix.cells[0].right +
        matrix.cells[1].left + matrix.cells[1].right + matrix.marker;
    if (matrix.cells[0].left != 10) return 1;
    if (matrix.cells[1].right != 32) return 2;
    if (checksum != 87) return 3;
    printf("%d %d %d\n", checksum, matrix.cells[1].right, matrix.marker);
    return 0;
}
