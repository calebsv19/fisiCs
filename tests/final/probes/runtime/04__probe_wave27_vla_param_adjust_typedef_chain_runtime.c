extern int printf(const char*, ...);

static int wave27_sum_adjusted_vla(int rows, int cols, int grid[static rows][cols]) {
    typedef int wave27_cell_t;
    typedef wave27_cell_t wave27_row_t[cols];
    typedef wave27_row_t* wave27_row_ptr_t;

    wave27_row_ptr_t view = grid;
    int total = 0;
    for (int r = 0; r < rows; ++r) {
        total += view[r][r % cols];
    }
    return total + (int)sizeof(view[0]);
}

static int wave27_vla_param_adjust_typedef_chain(void) {
    int rows = 4;
    int cols = 6;
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 40 + r * 8 + c;
        }
    }
    return wave27_sum_adjusted_vla(rows, cols, grid);
}

int main(void) {
    printf("%d\n", wave27_vla_param_adjust_typedef_chain());
    return 0;
}
