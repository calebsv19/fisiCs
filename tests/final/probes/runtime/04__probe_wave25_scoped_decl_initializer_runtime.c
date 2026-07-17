extern int printf(const char*, ...);

static int wave25_scoped_decl_initializer(int rows, int cols) {
    int matrix[rows][cols];
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            matrix[r][c] = r * 50 + c;
        }
    }

    int total = 0;
    {
        typedef int row_t[cols];
        row_t* row_ref = &matrix[1];
        int (*cell_view)[cols] = row_ref;
        int selected = cell_view[0][cols - 1];
        total += selected + (int)(sizeof(*row_ref) / sizeof((*row_ref)[0]));
    }
    {
        typedef int row_t[cols + 1];
        int scratch[cols + 1];
        row_t* scratch_ref = &scratch;
        for (int i = 0; i < cols + 1; ++i) {
            (*scratch_ref)[i] = total + i;
        }
        int scoped_init[2] = {
            (*scratch_ref)[cols],
            (int)(sizeof(*scratch_ref) / sizeof((*scratch_ref)[0])),
        };
        total += scoped_init[0] + scoped_init[1];
    }

    return total;
}

int main(void) {
    printf("%d\n", wave25_scoped_decl_initializer(3, 4));
    return 0;
}
