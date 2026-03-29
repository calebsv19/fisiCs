#include <stdio.h>

static int store_order(
    int n,
    int* restrict dst,
    const int* restrict src,
    volatile int* trace
) {
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        int v = src[i] + (i * 2);
        dst[i] = v;
        trace[i & 3] = trace[i & 3] * 3 + v;
        acc += dst[i] + (int)trace[i & 3];
    }
    return acc;
}

int main(void) {
    int src[8] = {2, 5, 7, 11, 13, 17, 19, 23};
    int out[8] = {0};
    volatile int trace[4] = {1, 2, 3, 4};

    int acc = store_order(8, out, src, trace);
    printf(
        "%d %d %d %d %d\n",
        out[0] + out[7],
        out[3] + out[4],
        acc,
        (int)(trace[0] + trace[1]),
        (int)(trace[2] + trace[3])
    );
    return 0;
}
