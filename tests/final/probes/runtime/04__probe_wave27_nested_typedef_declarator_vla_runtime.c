extern int printf(const char*, ...);

static int wave27_nested_typedef_declarator_vla(int rows, int cols) {
    typedef int wave27_scalar_t;
    typedef wave27_scalar_t wave27_row_t[cols];
    typedef wave27_row_t wave27_grid_t[rows];
    typedef wave27_row_t* wave27_row_view_t;
    typedef wave27_grid_t* wave27_grid_ref_t;

    wave27_grid_t grid;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 30 + r * 9 + c;
        }
    }

    wave27_grid_ref_t ref = &grid;
    wave27_row_view_t view = *ref;
    int values[3] = {
        view[0][cols - 1],
        view[rows - 1][1],
        (int)sizeof(view[0]),
    };
    return values[0] + values[1] * 2 + values[2];
}

int main(void) {
    printf("%d\n", wave27_nested_typedef_declarator_vla(3, 5));
    return 0;
}
