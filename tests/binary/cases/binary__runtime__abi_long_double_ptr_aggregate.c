#include <stdio.h>

typedef struct {
    long double gain;
    int *buf;
    int n;
} W17Node;

static long long quant8(long double x) {
    if (x >= 0.0L) return (long long)(x * 8.0L + 0.5L);
    return (long long)(x * 8.0L - 0.5L);
}

static long long score(W17Node value) {
    long double acc = 0.25L;
    int i;
    for (i = 0; i < value.n; ++i) {
        acc += (long double)value.buf[i] * value.gain * (long double)(i + 1);
    }
    return quant8(acc);
}

int main(void) {
    int data[3];
    W17Node node;
    data[0] = 2;
    data[1] = -1;
    data[2] = 4;
    node.gain = 1.5L;
    node.buf = data;
    node.n = 3;
    printf("%lld\n", score(node));
    return 0;
}
