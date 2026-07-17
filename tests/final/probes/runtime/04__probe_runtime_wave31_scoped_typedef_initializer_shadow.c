extern int printf(const char*, ...);

static int wave31_scoped_typedef_initializer_shadow(int rows, int cols) {
    typedef int wave31_cell_t;
    wave31_cell_t seed = 7;
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = seed + r * 11 + c;
        }
    }

    int total = 0;
    {
        typedef wave31_cell_t wave31_row_t[cols];
        typedef wave31_row_t* wave31_view_t;
        wave31_view_t first = grid, second = first + 1;
        int init[5] = {
            first[0][cols - 1],
            second[1][0],
            (int)sizeof(first[0]),
            (int)(second - first),
            seed,
        };
        total = init[0] + init[1] + init[2] + init[3] + init[4];
    }

    {
        typedef int wave31_cell_t;
        wave31_cell_t shadow = total / cols;
        total += shadow + grid[rows - 1][1];
    }
    return total;
}

int main(void) {
    printf("%d\n", wave31_scoped_typedef_initializer_shadow(3, 5));
    return 0;
}
