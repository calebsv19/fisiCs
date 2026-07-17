extern int printf(const char*, ...);

static int wave24_pick_from_rows_current(int cols, int (*rows)[cols]) {
    return rows[1][2] + rows[0][3];
}

static int wave24_vla_pointer_direct_chooser_current(int rows, int cols) {
    int left[rows][cols];
    int right[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            left[r][c] = 100 + r * 10 + c;
            right[r][c] = 300 + r * 10 + c;
        }
    }

    (void)left;
    return wave24_pick_from_rows_current(cols, right) + cols;
}

int main(void) {
    printf("%d\n", wave24_vla_pointer_direct_chooser_current(2, 4));
    return 0;
}
