extern int printf(const char*, ...);

int wave21_vla_pointer_chain_sum(int plane_count, int cols, int (*planes[plane_count])[cols]);

int wave21_vla_pointer_chain_sum(int plane_count, int cols, int (*(*planes))[cols]) {
    int total = 0;
    int i = 0;
    for (i = 0; i < plane_count; ++i) {
        total += planes[i][0][i];
        total += planes[i][1][cols - 1 - i];
    }
    return total;
}

int main(void) {
    int cols = 4;
    int left[2][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
    };
    int right[2][4] = {
        {10, 11, 12, 13},
        {14, 15, 16, 17},
    };
    int (*planes[2])[4] = {left, right};
    printf("%d\n", wave21_vla_pointer_chain_sum(2, cols, planes));
    return 0;
}
