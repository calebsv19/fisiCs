#include <stdio.h>

typedef struct {
    int *ptr;
    int len;
} Span;

typedef struct {
    Span s;
    int bias;
} SpanWrap;

SpanWrap abi_make_span_wrap(int *base, int len, int bias);
int abi_score_span_wrap(SpanWrap value);

int main(void) {
    int data[4];
    data[0] = 3;
    data[1] = 1;
    data[2] = 4;
    data[3] = 1;
    printf("%d\n", abi_score_span_wrap(abi_make_span_wrap(data, 4, 7)));
    return 0;
}
