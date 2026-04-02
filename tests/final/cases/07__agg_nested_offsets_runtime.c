#include <stddef.h>
#include <stdio.h>

struct Inner {
    short s;
    int i;
};

struct Outer {
    char c;
    struct Inner inner;
    long l;
};

int main(void) {
    size_t off_inner = offsetof(struct Outer, inner);
    size_t off_inner_i = offsetof(struct Outer, inner.i);
    size_t off_l = offsetof(struct Outer, l);
    int ordered = (off_inner < off_inner_i) && (off_inner_i < off_l);
    printf("%d %zu %zu %zu\n", ordered, off_inner, off_inner_i, off_l);
    return 0;
}
