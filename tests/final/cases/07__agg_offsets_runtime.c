#include <stddef.h>
#include <stdio.h>

struct Layout {
    char c;
    int i;
    short s;
};

int main(void) {
    size_t off_c = offsetof(struct Layout, c);
    size_t off_i = offsetof(struct Layout, i);
    size_t off_s = offsetof(struct Layout, s);
    int ordered = (off_c < off_i) && (off_i < off_s);
    printf("%d %zu %zu %zu\n", ordered, off_c, off_i, off_s);
    return 0;
}
