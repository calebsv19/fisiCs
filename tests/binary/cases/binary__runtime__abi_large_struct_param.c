#include <stdio.h>

typedef struct {
    int a[8];
    double d[4];
    long long tail;
} Big;

static long long score(Big value) {
    int i;
    long long acc = 0;
    for (i = 0; i < 8; ++i) {
        acc += (long long)(i + 1) * (long long)value.a[i];
    }
    for (i = 0; i < 4; ++i) {
        acc += (long long)(value.d[i] * 8.0);
    }
    acc += value.tail * 3LL;
    return acc;
}

int main(void) {
    Big value;
    int i;

    for (i = 0; i < 8; ++i) {
        value.a[i] = i + 1;
    }
    value.d[0] = 1.25;
    value.d[1] = 2.5;
    value.d[2] = 3.75;
    value.d[3] = 4.0;
    value.tail = 9LL;

    printf("%lld\n", score(value));
    return 0;
}
