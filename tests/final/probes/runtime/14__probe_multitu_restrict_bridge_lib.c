int restrict_bridge_mix(int *restrict dst, const int *restrict left, const int *restrict right, int n) {
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        dst[i] = left[i] - right[i] + i;
        acc += dst[i];
    }
    return acc;
}
