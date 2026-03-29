#include <stdio.h>

int restrict_bridge_mix(int *restrict dst, const int *restrict left, const int *restrict right, int n);

int main(void) {
    int left[6] = {9, 8, 7, 6, 5, 4};
    int right[6] = {1, 1, 2, 3, 5, 8};
    int dst[6] = {0, 0, 0, 0, 0, 0};
    int acc = restrict_bridge_mix(dst, left, right, 6);
    printf("%d %d %d\n", acc, dst[0] + dst[5], dst[2] * dst[3]);
    return 0;
}
