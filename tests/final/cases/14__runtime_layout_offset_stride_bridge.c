#include <stddef.h>
#include <stdio.h>

struct Inner {
    char c;
    int x;
    short y;
};

struct Outer {
    double d;
    struct Inner in[3];
    char tail;
};

int main(void) {
    struct Outer o;
    o.d = 1.0;
    o.in[0].c = 1;
    o.in[1].x = 22;
    o.in[2].y = 7;
    o.tail = 3;

    size_t off_in = offsetof(struct Outer, in);
    size_t off_tail = offsetof(struct Outer, tail);
    size_t off_x = offsetof(struct Inner, x);
    size_t sz_outer = sizeof(struct Outer);
    size_t sz_inner = sizeof(struct Inner);
    ptrdiff_t stride = (char *)&o.in[2] - (char *)&o.in[0];

    printf("%zu %zu %zu %zu %zu %td\n",
           off_in, off_tail, off_x, sz_outer, sz_inner, stride);
    return 0;
}
