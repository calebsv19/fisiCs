extern int printf(const char*, ...);

static int wave29_pick_edge(int cols, int (*rows)[cols]) {
    return rows[0][cols - 1] + rows[2][1];
}

static int wave29_pick_fold(int cols, int (*rows)[cols]) {
    return rows[1][2] * 2 - rows[0][0];
}

static int wave29_nested_fnptr_vla_dispatch(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 11 + r * 5 + c;
        }
    }

    typedef int wave29_cell_t;
    typedef wave29_cell_t wave29_row_t[cols];
    typedef wave29_row_t* wave29_view_t;
    typedef int (*wave29_picker_t)(int, wave29_view_t);
    typedef wave29_picker_t wave29_picker_table_t[2];
    typedef wave29_picker_table_t* wave29_table_view_t;

    wave29_picker_table_t table = {wave29_pick_edge, wave29_pick_fold};
    wave29_table_view_t table_view = &table;
    wave29_view_t view = grid;
    int selected[3] = {
        (*table_view)[0](cols, view),
        (*table_view)[1](cols, view),
        (int)(sizeof(table) / sizeof(table[0])),
    };
    return selected[0] + selected[1] + selected[2];
}

int main(void) {
    printf("%d\n", wave29_nested_fnptr_vla_dispatch(3, 4));
    return 0;
}
