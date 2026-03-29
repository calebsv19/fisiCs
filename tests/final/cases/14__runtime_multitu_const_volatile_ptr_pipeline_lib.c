int abi_cv_reduce(const int* xs, int n, int bias) {
    int acc = bias;
    for (int i = 0; i < n; ++i) {
        acc += xs[i] * (i + 1);
    }
    return acc;
}

int abi_cv_window(const int* xs, int n, int base, int width) {
    int acc = 0;
    for (int i = 0; i < width; ++i) {
        int idx = (base + i) % n;
        acc += xs[idx] * (i + 2);
    }
    return acc;
}
