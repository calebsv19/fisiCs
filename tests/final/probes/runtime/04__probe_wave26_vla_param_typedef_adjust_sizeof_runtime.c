extern int printf(const char*, ...);

static int wave26_sum_vla_param(int rows, int cols, int grid[rows][cols]) {
    typedef int wave26_row_t[cols];
    wave26_row_t* view = grid;
    int total = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            total += view[r][c];
        }
    }
    return total + (int)sizeof(view[0]);
}

static int wave26_vla_param_typedef_adjust_sizeof(void) {
    int rows = 3;
    int cols = 4;
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = r * 10 + c;
        }
    }
    return wave26_sum_vla_param(rows, cols, grid);
}

int main(void) {
    printf("%d\n", wave26_vla_param_typedef_adjust_sizeof());
    return 0;
}
