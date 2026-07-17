extern int printf(const char*, ...);

static int wave29_initializer_declarator_vla_views(int rows, int cols) {
    int grid[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            grid[r][c] = 20 + r * 3 + c;
        }
    }

    typedef int wave29_scalar_t;
    typedef wave29_scalar_t wave29_row_t[cols];
    typedef wave29_row_t* wave29_view_t;

    wave29_view_t first = grid, second = first + 1;
    int init[4] = {
        first[0][cols - 1],
        second[1][0],
        (int)sizeof(first[0]),
        (int)(second - first),
    };
    return init[0] + init[1] + init[2] + init[3];
}

int main(void) {
    printf("%d\n", wave29_initializer_declarator_vla_views(3, 5));
    return 0;
}
