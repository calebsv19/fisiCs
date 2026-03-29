#include <stddef.h>
#include <stdio.h>

static long long reduce_block(int cols, int rows, int block[rows][cols]) {
    long long acc = 0;
    for (int r = 0; r < rows; ++r) {
        int idx = (r * 2 + 1) % cols;
        acc += (long long)block[r][idx] * (long long)(r + 3);
    }
    ptrdiff_t s1 = &block[1][0] - &block[0][0];
    ptrdiff_t s2 = &block[rows - 1][cols - 1] - &block[0][1];
    return acc + (long long)s1 + (long long)s2;
}

static long long handoff(int rows, int cols, int m[rows][cols], int pivot) {
    int (*base)[cols] = &m[pivot];
    long long a = reduce_block(cols, 3, base - 1);
    long long b = reduce_block(cols, 2, base);
    return a * 2 + b;
}

int main(void) {
    int rows = 8;
    int cols = 7;
    int m[rows][cols];

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            m[r][c] = r * 23 - c * 4 + 11;
        }
    }

    long long out = handoff(rows, cols, m, 3);
    ptrdiff_t span = &m[7][6] - &m[2][3];
    printf("%lld %lld\n", out, (long long)span);
    return 0;
}
