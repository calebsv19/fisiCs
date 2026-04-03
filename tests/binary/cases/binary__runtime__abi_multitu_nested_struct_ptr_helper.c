typedef struct {
    int *ptr;
    int len;
} Span;

typedef struct {
    Span s;
    int bias;
} SpanWrap;

SpanWrap abi_make_span_wrap(int *base, int len, int bias) {
    SpanWrap value;
    value.s.ptr = base;
    value.s.len = len;
    value.bias = bias;
    return value;
}

int abi_score_span_wrap(SpanWrap value) {
    int i;
    int acc = value.bias;
    for (i = 0; i < value.s.len; ++i) {
        acc += value.s.ptr[i] * (i + 1);
    }
    return acc;
}
