extern int printf(const char*, ...);

static int wave24_scoped_typedef_vla_sizeof_initializer(int rows, int cols) {
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
        (int)(sizeof(view[0]) / sizeof(view[0][0])),
    };
    int guard[(sizeof(row_t) == sizeof(view[0])) ? 1 : 2];

    return picks[0] + picks[1] + picks[2] + (int)sizeof(row_t) +
           (int)(sizeof(guard) / sizeof(guard[0]));
}

int main(void) {
    printf("%d\n", wave24_scoped_typedef_vla_sizeof_initializer(3, 5));
    return 0;
}
