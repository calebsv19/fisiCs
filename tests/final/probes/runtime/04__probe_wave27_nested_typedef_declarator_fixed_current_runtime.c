extern int printf(const char*, ...);

static int wave27_nested_typedef_declarator_fixed_current(void) {
    typedef int wave27_scalar_t;
    typedef wave27_scalar_t wave27_row_t[5];
    typedef wave27_row_t wave27_grid_t[3];
    typedef wave27_row_t* wave27_row_view_t;

    wave27_grid_t grid;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 5; ++c) {
            grid[r][c] = 30 + r * 9 + c;
        }
    }

    wave27_row_view_t view = grid;
    int values[3] = {
        view[0][4],
        view[2][1],
        (int)sizeof(view[0]),
    };
    return values[0] + values[1] * 2 + values[2];
}

int main(void) {
    printf("%d\n", wave27_nested_typedef_declarator_fixed_current());
    return 0;
}
