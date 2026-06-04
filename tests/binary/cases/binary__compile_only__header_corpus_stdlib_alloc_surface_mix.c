#include <stddef.h>
#include <stdlib.h>

void *wave26_stdlib_alloc_surface(size_t count) {
    void *p = calloc(count, sizeof(int));
    void *q = realloc(p, count * sizeof(long));

    if (!q) {
        free(p);
        return 0;
    }

    free(q);
    return malloc(count);
}

int main(void) {
    void *p = wave26_stdlib_alloc_surface(4);
    free(p);
    return 0;
}
