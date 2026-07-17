int main(void) {
    int (*broken)(int;
    {
        int cols = 3;
        int rows = 2;
        int grid[2][3] = {{2, 4, 6}, {3, 5, 7}};
        int (*row)[cols] = grid;
        return row[rows - 1][cols - 1];
    }
}
