extern int printf(const char*, ...);

static int wave31_pick_left(int cols, int (*rows)[cols]) {
    return rows[0][0] + rows[1][cols - 1];
}

static int wave31_pick_right(int cols, int (*rows)[cols]) {
    return rows[2][1] + rows[0][cols - 2];
}

static int wave31_declarator_initializer_ordering(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 50 + r * 5 + c;
        }
    }

    typedef int wave31_cell_t;
    typedef wave31_cell_t wave31_row_t[cols];
    typedef wave31_row_t* wave31_view_t;
    typedef int (*wave31_pick_t)(int, wave31_view_t);

    wave31_view_t base = grid, shifted = base + 1;
    wave31_pick_t picks[2] = {wave31_pick_left, wave31_pick_right}, selected = picks[1];
    int init[5] = {
        picks[0](cols, base),
        selected(cols, base),
        shifted[1][2],
        (int)sizeof(base[0]),
        (int)(shifted - base),
    };
    return init[0] + init[1] + init[2] + init[3] + init[4];
}

int main(void) {
    printf("%d\n", wave31_declarator_initializer_ordering(3, 4));
    return 0;
}
