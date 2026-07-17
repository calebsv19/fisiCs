extern int printf(const char*, ...);

typedef int (*wave32_reader_t)(int, int (*)[*]);
typedef wave32_reader_t (*wave32_factory_t)(int);

static int wave32_read_diag(int cols, int (*rows)[cols]) {
    return rows[0][0] + rows[1][1] + rows[2][2];
}

static int wave32_read_edges(int cols, int (*rows)[cols]) {
    return rows[0][cols - 1] + rows[2][0] + rows[2][cols - 2];
}

static wave32_reader_t wave32_pick_reader(int which) {
    wave32_reader_t choices[2] = {wave32_read_diag, wave32_read_edges};
    return choices[which & 1];
}

static int wave32_factory_returns_vla_reader(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 17 + r * 9 + c * 2;
        }
    }

    typedef int wave32_scalar_t;
    typedef wave32_scalar_t wave32_row_t[cols];
    typedef wave32_row_t* wave32_view_t;
    wave32_factory_t factory = wave32_pick_reader;
    wave32_view_t view = grid;
    wave32_reader_t a = factory(0);
    wave32_reader_t b = factory(1);
    int result[4] = {
        a(cols, view),
        b(cols, view),
        view[1][cols - 1],
        (int)sizeof(view[0]),
    };
    return result[0] + result[1] + result[2] + result[3];
}

int main(void) {
    printf("%d\n", wave32_factory_returns_vla_reader(3, 5));
    return 0;
}
