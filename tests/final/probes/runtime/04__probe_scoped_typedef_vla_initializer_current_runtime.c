extern int printf(const char*, ...);

static int wave24_scoped_typedef_vla_initializer_current(int rows, int cols) {
    int matrix[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            matrix[r][c] = r * 100 + c;
        }
    }

    typedef int row_t[cols];
    row_t* view = matrix;
    int picks[3] = {
        view[0][cols - 1],
        view[rows - 1][0],
        cols,
    };

    return picks[0] + picks[1] + picks[2] + 1;
}

int main(void) {
    printf("%d\n", wave24_scoped_typedef_vla_initializer_current(3, 5));
    return 0;
}
