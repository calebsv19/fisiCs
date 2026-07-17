extern int printf(const char*, ...);

struct Row {
    int cells[2];
};

struct Grid {
    struct Row rows[2];
    int bias;
};

int main(void) {
    struct Grid grid = {{{3, 5}, {7, 11}}, 4};
    struct Row *row = &grid.rows[1];

    row->cells[0] += grid.bias;
    grid.rows[0].cells[1] *= row->cells[0] - 6;
    grid.rows[1].cells[1] -= grid.rows[0].cells[0];

    printf("%d %d %d %d\n", grid.rows[0].cells[0], grid.rows[0].cells[1],
           grid.rows[1].cells[0], grid.rows[1].cells[1]);
    return 0;
}
