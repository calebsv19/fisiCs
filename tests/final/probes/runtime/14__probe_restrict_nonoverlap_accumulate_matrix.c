#include <stdio.h>

static int fold_add_mul(int n, int *restrict dst, const int *restrict a, const int *restrict b) {
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        dst[i] = a[i] + (2 * b[i]);
        acc += dst[i] * (i + 1);
    }
    return acc;
}

int main(void) {
    int a[4] = {1, 3, 5, 7};
    int b[4] = {2, 4, 6, 8};
    int out[4] = {0, 0, 0, 0};
    int acc = fold_add_mul(4, out, a, b);
    printf("%d %d %d\n", out[0] + out[3], out[1] + out[2], acc);
    return 0;
}
