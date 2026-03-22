#include <stddef.h>
#include <stdio.h>

static long long reduce_matrix(int rows, int cols, int m[rows][cols]) {
    int mid = rows / 2;
    int (*base)[cols] = &m[mid];
    int (*prev)[cols] = base - 1;
    int (*next)[cols] = base + 1;

    ptrdiff_t d_prev = &prev[0][2] - &base[0][2];
    ptrdiff_t d_next = &next[0][1] - &base[0][1];

    size_t u = 3;
    int a = base[0][u];
    int b = prev[0][u - 1];
    int c = next[0][u - 2];

    return (long long)a + (long long)b + (long long)c + (long long)d_prev + (long long)d_next;
}

int main(void) {
    int rows = 7;
    int cols = 6;
    int m[rows][cols];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            m[i][j] = i * 40 + j * 3 + 1;
        }
    }

    long long out = reduce_matrix(rows, cols, m);
    ptrdiff_t span1 = &m[1][0] - &m[4][0];
    ptrdiff_t span2 = &m[6][5] - &m[2][2];
    printf("%lld %lld %lld\n", out, (long long)span1, (long long)span2);
    return 0;
}

