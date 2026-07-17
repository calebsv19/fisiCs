typedef int (*wave29_callback_t)(int);

static int wave29_identity(int value) {
    return value;
}

int wave29_accept(int n, int (*rows)[n], wave29_callback_t callbacks[static 1]) {
    typedef int wave29_row_t[n];
    typedef wave29_row_t* wave29_view_t;
    typedef int (*wave29_picker_t)(int, wave29_view_t);

    wave29_view_t view = rows;
    wave29_callback_t cb = callbacks[0];
    wave29_picker_t picker = 0;
    return cb(view[0][0]) + (picker == 0);
}

int main(void) {
    int grid[2][3] = {{1, 2, 3}, {4, 5, 6}};
    wave29_callback_t callbacks[1] = {wave29_identity};
    return wave29_accept(3, grid, callbacks);
}
