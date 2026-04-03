struct W17Span {
    long double scale;
    int *ptr;
    unsigned n;
};

long long w17_step2(struct W17Span span, unsigned split);

long long w17_step1(struct W17Span span) {
    return w17_step2(span, 2u) + 5ll;
}
