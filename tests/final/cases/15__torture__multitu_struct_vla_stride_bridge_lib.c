struct Lane {
    int base;
    int stride;
};

static int lane_weight(struct Lane lane, int row, int col) {
    int w = lane.base + row * lane.stride + col;
    return (w % 17) + 1;
}

int mt13_vla_fold(int rows, int cols, int grid[rows][cols], struct Lane lane) {
    int total = 0;

    for (int r = 0; r < rows; ++r) {
        int *rowp = grid[r];
        for (int c = 0; c < cols; ++c) {
            total += rowp[c] * lane_weight(lane, r, c);
        }
    }

    return total;
}

int mt13_vla_edge(int rows, int cols, int grid[rows][cols]) {
    int *start = &grid[0][0];
    int *end = &grid[rows - 1][cols - 1];
    int span = (int)(end - start);
    int diag = 0;

    for (int i = 0; i < rows && i < cols; ++i) {
        diag += grid[i][i];
    }

    return span + diag;
}
