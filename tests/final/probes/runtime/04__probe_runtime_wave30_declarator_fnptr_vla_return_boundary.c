extern int printf(const char*, ...);

typedef int (*wave30_reader_t)(int, int (*)[*]);

static int wave30_read_diag(int cols, int (*rows)[cols]) {
    return rows[0][0] + rows[1][1] + rows[2][2];
}

static int wave30_read_edge(int cols, int (*rows)[cols]) {
    return rows[0][cols - 1] + rows[2][0];
}

static wave30_reader_t wave30_choose_reader(int which) {
    wave30_reader_t table[2] = {wave30_read_diag, wave30_read_edge};
    return table[which & 1];
}

static int wave30_declarator_fnptr_vla_return_boundary(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 40 + r * 4 + c * 2;
        }
    }

    typedef int wave30_scalar_t;
    typedef wave30_scalar_t wave30_row_t[cols];
    typedef wave30_row_t* wave30_view_t;
    typedef wave30_reader_t (*wave30_factory_t)(int);

    wave30_factory_t factory = wave30_choose_reader;
    wave30_view_t view = grid;
    wave30_reader_t first = factory(0), second = factory(1);
    int init[4] = {
        first(cols, view),
        second(cols, view),
        (int)sizeof(view[0]),
        view[1][2],
    };
    return init[0] + init[1] + init[2] + init[3];
}

int main(void) {
    printf("%d\n", wave30_declarator_fnptr_vla_return_boundary(3, 4));
    return 0;
}
