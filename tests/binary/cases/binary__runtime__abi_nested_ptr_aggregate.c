#include <stdio.h>

typedef struct {
    int *p;
    int n;
} Slice;

typedef struct {
    Slice left;
    Slice right;
} PairSlice;

static int weighted_sum(PairSlice value) {
    int i;
    int acc = 0;
    for (i = 0; i < value.left.n; ++i) {
        acc += value.left.p[i] * (i + 1);
    }
    for (i = 0; i < value.right.n; ++i) {
        acc += value.right.p[i] * (i + 2);
    }
    return acc;
}

int main(void) {
    int a[3];
    int b[2];
    PairSlice value;

    a[0] = 2;
    a[1] = 4;
    a[2] = 6;
    b[0] = 5;
    b[1] = 7;

    value.left.p = a;
    value.left.n = 3;
    value.right.p = b;
    value.right.n = 2;

    printf("%d\n", weighted_sum(value));
    return 0;
}
