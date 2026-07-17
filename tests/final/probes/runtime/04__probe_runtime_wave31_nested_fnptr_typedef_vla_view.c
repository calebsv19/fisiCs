extern int printf(const char*, ...);

static int wave31_sum_corner(int cols, int (*rows)[cols]) {
    return rows[0][0] + rows[2][cols - 1];
}

static int wave31_sum_middle(int cols, int (*rows)[cols]) {
    return rows[1][1] + rows[2][2];
}

static int wave31_nested_fnptr_typedef_vla_view(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 9 + r * 10 + c;
        }
    }

    typedef int wave31_cell_t;
    typedef wave31_cell_t wave31_row_t[cols];
    typedef wave31_row_t* wave31_view_t;
    typedef int (*wave31_reader_t)(int, wave31_view_t);
    typedef wave31_reader_t wave31_reader_pair_t[2];
    typedef wave31_reader_pair_t* wave31_reader_table_t;

    wave31_reader_pair_t pair = {wave31_sum_corner, wave31_sum_middle};
    wave31_reader_table_t table = &pair;
    wave31_view_t view = grid;
    int values[4] = {
        (*table)[0](cols, view),
        (*table)[1](cols, view),
        view[1][cols - 1],
        (int)sizeof(view[0]),
    };
    return values[0] + values[1] + values[2] + values[3];
}

int main(void) {
    printf("%d\n", wave31_nested_fnptr_typedef_vla_view(3, 5));
    return 0;
}
