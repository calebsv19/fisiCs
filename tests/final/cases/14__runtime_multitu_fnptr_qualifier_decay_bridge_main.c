#include <stdio.h>

typedef int (*ReduceFn)(const int* xs, int n);

int abi_reduce_apply(ReduceFn fn, const int* xs, int n);
int abi_reduce_dispatch(const int* xs, int n, int mode);

static int reduce_tail(const int* xs, int n) {
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        acc += xs[i] * (n - i);
    }
    return acc;
}

int main(void) {
    const int xs[6] = {2, 7, 1, 8, 2, 8};
    int a = abi_reduce_dispatch(xs, 6, 0);
    int b = abi_reduce_dispatch(xs, 6, 1);
    int c = abi_reduce_apply(reduce_tail, xs, 6);
    printf("%d %d %d\n", a, b, c);
    return 0;
}
