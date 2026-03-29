typedef int (*ReduceFn)(const int* xs, int n);

static int reduce_even_weighted(const int* xs, int n) {
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        int w = (i % 2 == 0) ? 3 : 1;
        acc += xs[i] * w;
    }
    return acc;
}

static int reduce_pair_mix(const int* xs, int n) {
    int acc = 0;
    for (int i = 0; i < n; ++i) {
        int prev = (i > 0) ? xs[i - 1] : 0;
        acc += xs[i] * (i + 1) - prev;
    }
    return acc;
}

int abi_reduce_apply(ReduceFn fn, const int* xs, int n) {
    return fn(xs, n);
}

int abi_reduce_dispatch(const int* xs, int n, int mode) {
    ReduceFn fn = (mode & 1) ? reduce_pair_mix : reduce_even_weighted;
    return abi_reduce_apply(fn, xs, n);
}
