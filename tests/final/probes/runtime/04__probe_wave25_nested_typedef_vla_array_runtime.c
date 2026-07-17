extern int printf(const char*, ...);

static int wave25_nested_typedef_vla_array(int rows, int cols) {
    int matrix[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            matrix[r][c] = (r + 1) * 10 + c;
        }
    }

    typedef int row_t[cols];
    typedef row_t plane_t[rows];
    plane_t* plane_ref = &matrix;
    int init[4] = {
        (*plane_ref)[rows - 1][cols - 1],
        (*plane_ref)[1][2],
        (int)(sizeof((*plane_ref)[0]) / sizeof((*plane_ref)[0][0])),
        (int)(sizeof(*plane_ref) / sizeof((*plane_ref)[0])),
    };

    return init[0] + init[1] + init[2] + init[3];
}

int main(void) {
    printf("%d\n", wave25_nested_typedef_vla_array(3, 4));
    return 0;
}
