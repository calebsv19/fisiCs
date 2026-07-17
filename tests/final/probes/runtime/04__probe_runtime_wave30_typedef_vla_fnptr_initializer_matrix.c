extern int printf(const char*, ...);

static int wave30_row_tail(int cols, int (*rows)[cols]) {
    return rows[1][cols - 1] + rows[2][0];
}

static int wave30_row_mix(int cols, int (*rows)[cols]) {
    return rows[0][1] * 2 + rows[2][cols - 2];
}

static int wave30_typedef_vla_fnptr_initializer_matrix(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 30 + r * 7 + c;
        }
    }

    typedef int wave30_cell_t;
    typedef wave30_cell_t wave30_row_t[cols];
    typedef wave30_row_t* wave30_view_t;
    typedef int (*wave30_pick_t)(int, wave30_view_t);
    typedef wave30_pick_t wave30_pick_row_t[2];
    typedef wave30_pick_row_t* wave30_pick_view_t;

    wave30_pick_row_t picks = {wave30_row_tail, wave30_row_mix};
    wave30_pick_view_t pick_view = &picks;
    wave30_view_t view = grid, offset = view + 1;
    int values[5] = {
        (*pick_view)[0](cols, view),
        (*pick_view)[1](cols, view),
        offset[0][1],
        (int)sizeof(view[0]),
        (int)(offset - view),
    };
    return values[0] + values[1] + values[2] + values[3] + values[4];
}

int main(void) {
    printf("%d\n", wave30_typedef_vla_fnptr_initializer_matrix(3, 5));
    return 0;
}
