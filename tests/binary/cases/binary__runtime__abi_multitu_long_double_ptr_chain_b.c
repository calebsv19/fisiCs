struct W17Span {
    long double scale;
    int *ptr;
    unsigned n;
};

static long long quant8_b(long double x) {
    if (x >= 0.0L) return (long long)(x * 8.0L + 0.5L);
    return (long long)(x * 8.0L - 0.5L);
}

long long w17_step3(struct W17Span span, unsigned start);

long long w17_step2(struct W17Span span, unsigned split) {
    long double acc = 0.0L;
    unsigned i;
    for (i = 0u; i < split && i < span.n; ++i) {
        acc += (long double)span.ptr[i] * span.scale * (long double)(i + 1u);
    }
    return quant8_b(acc) + w17_step3(span, split);
}
