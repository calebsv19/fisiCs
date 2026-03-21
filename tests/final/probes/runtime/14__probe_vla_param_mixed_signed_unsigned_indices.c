#include <stddef.h>
#include <stdio.h>

static int reduce_mixed(int rows, int cols, const int m[rows][cols]) {
    int sum = 0;
    for (unsigned ui = 0; ui < (unsigned)rows; ++ui) {
        for (int sj = cols - 1; sj >= 0; --sj) {
            size_t j = (size_t)sj;
            sum += m[ui][j] * (int)(ui + 1u);
        }
    }
    return sum;
}

int main(void) {
    int rows = 3;
    int cols = 4;
    int m[rows][cols];

    for (int i = 0; i < rows; ++i) {
        for (unsigned uj = 0; uj < (unsigned)cols; ++uj) {
            m[i][uj] = i * 10 + (int)uj;
        }
    }

    int total = reduce_mixed(rows, cols, m);
    ptrdiff_t span = &m[2][3] - &m[0][0];
    int edge = m[(unsigned)(rows - 1)][(size_t)(cols - 1)];

    printf("%d %td %d\n", total, span, edge);
    return 0;
}
