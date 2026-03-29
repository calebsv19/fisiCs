#include <stdio.h>

static int stage_mix(
    int n,
    int* restrict dst,
    const int* restrict a,
    const int* restrict b
) {
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        dst[i] = a[i] * 2 - b[i];
        acc += dst[i] * (i + 1);
    }
    return acc;
}

static int rebind_chain(
    int n,
    int* restrict dst,
    const int* restrict a,
    const int* restrict b
) {
    int split = n / 2;
    int left = stage_mix(split, dst, a, b);
    int right = stage_mix(n - split, dst + split, a + split, b + split);
    return left + right;
}

int main(void) {
    int a[10] = {3, 6, 9, 12, 15, 18, 21, 24, 27, 30};
    int b[10] = {1, 4, 7, 10, 13, 16, 19, 22, 25, 28};
    int out[10] = {0};

    int acc = rebind_chain(10, out, a, b);
    printf("%d %d %d\n", out[0] + out[9], out[4] + out[5], acc);
    return 0;
}
