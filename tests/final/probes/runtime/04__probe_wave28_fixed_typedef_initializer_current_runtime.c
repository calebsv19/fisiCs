extern int printf(const char*, ...);

static int wave28_fixed_sum(int (*rows)[4]) {
    return rows[0][3] + rows[2][1];
}

static int wave28_fixed_typedef_initializer_current(void) {
    int grid[3][4];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            grid[r][c] = 30 + r * 6 + c;
        }
    }

    typedef int wave28_fixed_row_t[4];
    typedef wave28_fixed_row_t* wave28_fixed_view_t;
    wave28_fixed_view_t view = grid;
    int init[3] = {
        wave28_fixed_sum(view),
        view[1][2],
        (int)(sizeof(view[0]) / sizeof(view[0][0])),
    };
    return init[0] + init[1] + init[2];
}

int main(void) {
    printf("%d\n", wave28_fixed_typedef_initializer_current());
    return 0;
}
