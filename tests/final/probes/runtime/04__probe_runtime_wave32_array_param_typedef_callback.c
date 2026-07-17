extern int printf(const char*, ...);

typedef int (*wave32_callback_t)(int, int [*], int (*)[*]);

static int wave32_callback_left(int cols, int values[static cols], int (*rows)[cols]) {
    return values[0] + values[cols - 1] + rows[0][2] + rows[2][1];
}

static int wave32_callback_right(int cols, int values[static cols], int (*rows)[cols]) {
    return values[1] + values[cols - 2] + rows[1][0] + rows[2][cols - 1];
}

static int wave32_array_param_typedef_callback(int rows, int cols) {
    int values[cols];
    int grid[rows][cols];
    for (int c = 0; c < cols; ++c) {
        values[c] = 11 + c * 3;
    }
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 30 + r * 8 + c;
        }
    }

    typedef wave32_callback_t wave32_callback_pair_t[2];
    typedef wave32_callback_pair_t* wave32_callback_table_t;
    wave32_callback_pair_t callbacks = {wave32_callback_left, wave32_callback_right};
    wave32_callback_table_t table = &callbacks;
    int parts[4] = {
        (*table)[0](cols, values, grid),
        (*table)[1](cols, values, grid),
        values[2],
        (int)sizeof(grid[0]),
    };
    return parts[0] + parts[1] + parts[2] + parts[3];
}

int main(void) {
    printf("%d\n", wave32_array_param_typedef_callback(3, 6));
    return 0;
}
