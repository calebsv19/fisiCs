extern int printf(const char*, ...);

static int wave32_shadow_sum(int cols, int (*rows)[cols]) {
    return rows[0][1] + rows[1][cols - 1] + rows[2][0];
}

static int wave32_scoped_shadow_vla_initializer(int rows, int cols) {
    typedef int wave32_cell_t;
    typedef wave32_cell_t wave32_outer_row_t[4];
    wave32_outer_row_t fixed = {3, 5, 7, 9};
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 21 + r * 6 + c;
        }
    }

    {
        typedef wave32_cell_t wave32_inner_row_t[cols];
        typedef wave32_inner_row_t* wave32_view_t;
        typedef int (*wave32_reader_t)(int, wave32_view_t);
        wave32_view_t view = grid;
        wave32_reader_t reader = wave32_shadow_sum;
        int parts[4] = {
            reader(cols, view),
            view[rows - 1][cols - 1],
            (int)sizeof(view[0]),
            fixed[2],
        };
        return parts[0] + parts[1] + parts[2] + parts[3];
    }
}

int main(void) {
    printf("%d\n", wave32_scoped_shadow_vla_initializer(3, 5));
    return 0;
}
