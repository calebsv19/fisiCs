#include <stdio.h>

typedef struct {
    int *base;
    int len;
} Span;

static int *offset_ptr(Span span, int off) {
    if (off < 0) return span.base;
    if (off >= span.len) return span.base + (span.len - 1);
    return span.base + off;
}

int main(void) {
    int values[6];
    Span span;
    int *p;

    values[0] = 4;
    values[1] = 8;
    values[2] = 15;
    values[3] = 16;
    values[4] = 23;
    values[5] = 42;

    span.base = values;
    span.len = 6;

    p = offset_ptr(span, 3);
    printf("%d\n", p[-1] + p[0] + p[1]);
    return 0;
}
