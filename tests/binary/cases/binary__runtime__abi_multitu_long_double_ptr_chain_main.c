#include <stdio.h>

struct W17Span {
    long double scale;
    int *ptr;
    unsigned n;
};

long long w17_step1(struct W17Span span);

int main(void) {
    int data[4];
    struct W17Span span;
    data[0] = 3;
    data[1] = 1;
    data[2] = 4;
    data[3] = 1;
    span.scale = 1.25L;
    span.ptr = data;
    span.n = 4u;
    printf("%lld\n", w17_step1(span));
    return 0;
}
