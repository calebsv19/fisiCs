extern int printf(const char*, ...);

static int wave25_pick_left(int cols, int (*rows)[cols]) {
    return rows[0][cols - 1] + rows[1][1];
}

static int wave25_pick_right(int cols, int (*rows)[cols]) {
    return rows[1][cols - 2] - rows[0][0];
}

static int wave25_fnptr_vla_typedef_dispatch(int rows, int cols) {
    int left[rows][cols];
    int right[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            left[r][c] = 100 + r * 10 + c;
            right[r][c] = 200 + r * 10 + c;
        }
    }

    typedef int row_t[cols];
    typedef row_t* row_ptr_t;
    typedef int (*picker_t)(int, row_ptr_t);
    typedef picker_t picker_table_t[2];

    picker_table_t table = {wave25_pick_left, wave25_pick_right};
    row_ptr_t rows_ref[2] = {left, right};
    int init[3] = {
        table[0](cols, rows_ref[0]),
        table[1](cols, rows_ref[1]),
        (int)(sizeof(table) / sizeof(table[0])),
    };

    return init[0] + init[1] + init[2];
}

int main(void) {
    printf("%d\n", wave25_fnptr_vla_typedef_dispatch(2, 5));
    return 0;
}
