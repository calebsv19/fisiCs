#include <stddef.h>
#include <stdio.h>

static long long eval_slice(int rows, int cols, int m[rows][cols], int start) {
    int (*p)[cols] = &m[start];
    unsigned ucol = 4u;
    int scol = -2;

    int left = p[1][ucol - 1u];
    int right = p[2][(int)ucol + scol];
    ptrdiff_t d1 = &p[2][(int)ucol + scol] - &p[1][ucol - 1u];
    ptrdiff_t d2 = &p[0][0] - &p[2][0];

    return (long long)left * 2LL + (long long)right * 3LL + (long long)d1 * 5LL + (long long)d2;
}

int main(void) {
    int rows = 8;
    int cols = 7;
    int m[rows][cols];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            m[i][j] = i * 100 + j * 4 + 3;
        }
    }

    long long r = eval_slice(rows, cols, m, 2);
    ptrdiff_t tail = &m[7][6] - &m[2][1];
    printf("%lld %lld\n", r, (long long)tail);
    return 0;
}

