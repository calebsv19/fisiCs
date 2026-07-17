typedef struct Cell46B {
    int left;
    int right;
} Cell46B;

typedef struct Matrix46B {
    Cell46B cells[2];
    int marker;
} Matrix46B;

typedef Matrix46B (*MatrixMaker46B)(Cell46B, int);

static Matrix46B make_scaled(Cell46B seed, int scale) {
    Matrix46B matrix;
    matrix.cells[0].left = seed.left + scale;
    matrix.cells[0].right = seed.right + scale;
    matrix.cells[1].left = seed.left * scale;
    matrix.cells[1].right = seed.right * scale;
    matrix.marker = scale + 5;
    return matrix;
}

static Matrix46B make_shifted(Cell46B seed, int scale) {
    Matrix46B matrix;
    matrix.cells[0].left = seed.left - scale;
    matrix.cells[0].right = seed.right - scale;
    matrix.cells[1].left = seed.left + scale * 2;
    matrix.cells[1].right = seed.right + scale * 2;
    matrix.marker = scale + 11;
    return matrix;
}

MatrixMaker46B wave46_select_matrix_maker(int route) {
    return (route & 1) ? make_scaled : make_shifted;
}
