extern int printf(const char*, ...);

static int wave26_left(int cols, int (*rows)[cols]) {
    return rows[0][cols - 1] + rows[1][0];
}

static int wave26_right(int cols, int (*rows)[cols]) {
    return rows[1][cols - 2] - rows[0][1];
}

static int wave26_typedef_declarator_boundary(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 20 + r * 7 + c;
        }
    }

    typedef int wave26_row_t[cols];
    typedef wave26_row_t* wave26_row_ptr_t;
    typedef int (*wave26_picker_t)(int, wave26_row_ptr_t);
    typedef wave26_picker_t (*wave26_picker_ref_t)[2];

    wave26_picker_t pickers[2] = {wave26_left, wave26_right};
    wave26_picker_ref_t ref = &pickers;
    wave26_row_ptr_t ptr = grid;
    int values[3] = {
        (*ref)[0](cols, ptr),
        (*ref)[1](cols, ptr),
        (int)(sizeof(*ref) / sizeof((*ref)[0])),
    };
    return values[0] * 3 + values[1] + values[2];
}

int main(void) {
    printf("%d\n", wave26_typedef_declarator_boundary(2, 5));
    return 0;
}
