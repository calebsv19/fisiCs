struct W17Span {
    long double scale;
    int *ptr;
    unsigned n;
};

static long long quant8_c(long double x) {
    if (x >= 0.0L) return (long long)(x * 8.0L + 0.5L);
    return (long long)(x * 8.0L - 0.5L);
}

long long w17_step3(struct W17Span span, unsigned start) {
    long double acc = 0.0L;
    unsigned i;
    for (i = start; i < span.n; ++i) {
        acc += (long double)span.ptr[i] * span.scale * (long double)(i + 1u);
    }
    return quant8_c(acc);
}
