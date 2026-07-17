extern int printf(const char*, ...);

static int wave28_pick_tail(int cols, int (*rows)[cols]) {
    return rows[1][cols - 1] + rows[2][1];
}

static int wave28_pick_fold(int cols, int (*rows)[cols]) {
    return rows[0][2] * 2 - rows[1][0];
}

static int wave28_typedef_vla_fnptr_initializer(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 10 + r * 7 + c;
        }
    }

    typedef int wave28_cell_t;
    typedef wave28_cell_t wave28_row_t[cols];
    typedef wave28_row_t* wave28_row_ptr_t;
    typedef int (*wave28_picker_t)(int, wave28_row_ptr_t);
    typedef wave28_picker_t wave28_picker_table_t[2];

    wave28_picker_table_t pickers = {wave28_pick_tail, wave28_pick_fold};
    wave28_row_ptr_t view = grid;
    int init[4] = {
        pickers[0](cols, view),
        pickers[1](cols, view),
        (int)sizeof(view[0]),
        (int)(sizeof(pickers) / sizeof(pickers[0])),
    };
    return init[0] + init[1] + init[2] + init[3];
}

int main(void) {
    printf("%d\n", wave28_typedef_vla_fnptr_initializer(3, 4));
    return 0;
}
