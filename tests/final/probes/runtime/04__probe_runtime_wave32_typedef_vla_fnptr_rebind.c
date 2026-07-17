extern int printf(const char*, ...);

typedef int (*wave32_rebind_reader_t)(int, int (*)[*]);

static int wave32_rebind_front(int cols, int (*rows)[cols]) {
    return rows[0][0] + rows[1][cols - 1];
}

static int wave32_rebind_back(int cols, int (*rows)[cols]) {
    return rows[2][1] + rows[3][cols - 2];
}

static int wave32_typedef_vla_fnptr_rebind(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 5 + r * 7 + c;
        }
    }

    typedef int wave32_cell_t;
    typedef wave32_cell_t wave32_row_t[cols];
    typedef wave32_row_t* wave32_view_t;
    typedef wave32_rebind_reader_t wave32_reader_pair_t[2];

    wave32_reader_pair_t readers = {wave32_rebind_front, wave32_rebind_back};
    wave32_view_t view = grid;
    wave32_rebind_reader_t first = readers[0];
    wave32_rebind_reader_t second = readers[1];
    int values[4] = {
        first(cols, view),
        second(cols, view),
        view[rows - 1][0],
        (int)sizeof(view[0]),
    };
    return values[0] + values[1] + values[2] + values[3];
}

int main(void) {
    printf("%d\n", wave32_typedef_vla_fnptr_rebind(4, 5));
    return 0;
}
