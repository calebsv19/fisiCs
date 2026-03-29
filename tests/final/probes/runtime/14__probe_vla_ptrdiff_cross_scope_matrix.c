#include <stddef.h>
#include <stdio.h>

static long long cross_scope(int rows, int cols, int m[rows][cols]) {
    long long acc = 0;

    for (int outer = 1; outer < rows - 1; ++outer) {
        int (*rowp)[cols] = &m[outer];
        {
            int left = rowp[0][1];
            int right = rowp[0][cols - 2];
            ptrdiff_t span = &m[outer + 1][0] - &m[outer - 1][0];
            acc += (long long)left * 3 + (long long)right * 2 + (long long)span;
        }
    }

    ptrdiff_t head = &m[rows - 1][cols - 1] - &m[0][0];
    ptrdiff_t mid;
    {
        int mid_r = rows / 2;
        mid = &m[mid_r][cols - 1] - &m[mid_r][0];
    }
    return acc + (long long)head + (long long)mid;
}

int main(void) {
    int rows = 9;
    int cols = 6;
    int m[rows][cols];

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            m[r][c] = r * 31 + c * 5 + 7;
        }
    }

    long long out = cross_scope(rows, cols, m);
    ptrdiff_t edge = &m[8][5] - &m[1][0];
    printf("%lld %lld\n", out, (long long)edge);
    return 0;
}
