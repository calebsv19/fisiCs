extern int printf(const char*, ...);

typedef int (*wave31_adjust_reader_t)(int, int [*], int (*)[*]);

static int wave31_read_front(int cols, int values[static cols], int (*rows)[cols]) {
    return values[0] + values[cols - 1] + rows[0][1] + rows[1][cols - 2];
}

static int wave31_read_back(int cols, int values[static cols], int (*rows)[cols]) {
    return values[1] + values[cols - 2] + rows[2][0] + rows[2][cols - 1];
}

static wave31_adjust_reader_t wave31_choose_adjust_reader(int which) {
    wave31_adjust_reader_t readers[2] = {wave31_read_front, wave31_read_back};
    return readers[which & 1];
}

static int wave31_array_param_fnptr_return_adjust(int rows, int cols) {
    int values[cols];
    int grid[rows][cols];
    for (int c = 0; c < cols; ++c) {
        values[c] = 3 + c * 2;
    }
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 20 + r * 6 + c;
        }
    }

    wave31_adjust_reader_t first = wave31_choose_adjust_reader(0);
    wave31_adjust_reader_t second = wave31_choose_adjust_reader(1);
    int parts[4] = {
        first(cols, values, grid),
        second(cols, values, grid),
        (int)sizeof(grid[0]),
        values[cols - 1],
    };
    return parts[0] + parts[1] + parts[2] + parts[3];
}

int main(void) {
    printf("%d\n", wave31_array_param_fnptr_return_adjust(3, 6));
    return 0;
}
