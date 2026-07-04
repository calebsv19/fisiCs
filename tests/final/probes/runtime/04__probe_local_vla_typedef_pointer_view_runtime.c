extern int printf(const char*, ...);

static int wave23_local_vla_typedef_view(int rows, int cols) {
    int matrix[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            matrix[r][c] = (r + 1) * 10 + c;
        }
    }

    typedef int row_t[cols];
    row_t* view = matrix;
    return view[0][1] + view[1][2] + view[2][3];
}

int main(void) {
    printf("%d\n", wave23_local_vla_typedef_view(3, 4));
    return 0;
}
