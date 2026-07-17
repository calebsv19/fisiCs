extern int printf(const char*, ...);

static int wave24_pick_from_rows(int cols, int (*rows)[cols]) {
    return rows[1][2] + rows[0][3];
}

static int wave24_vla_pointer_fnptr_chooser_nested(int rows, int cols) {
    int left[rows][cols];
    int right[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            left[r][c] = 100 + r * 10 + c;
            right[r][c] = 300 + r * 10 + c;
        }
    }

    typedef int row_t[cols];
    typedef row_t* row_ptr_t;
    typedef int (*picker_t)(int, row_ptr_t);

    picker_t picker = wave24_pick_from_rows;
    row_ptr_t selected = right;
    return picker(cols, selected) +
           (int)(sizeof(selected[0]) / sizeof(selected[0][0]));
}

int main(void) {
    printf("%d\n", wave24_vla_pointer_fnptr_chooser_nested(2, 4));
    return 0;
}
