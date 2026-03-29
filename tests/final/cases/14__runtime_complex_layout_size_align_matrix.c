#include <stdio.h>
#include <stddef.h>

typedef struct {
    char c;
    _Complex double x;
} align_probe;

typedef struct {
    _Complex double a;
    int tag;
    _Complex double b;
} layout_probe;

int main(void) {
    printf("%zu %zu %zu %zu %zu\n",
           sizeof(_Complex double),
           offsetof(align_probe, x),
           sizeof(layout_probe),
           offsetof(layout_probe, tag),
           offsetof(layout_probe, b));
    return 0;
}
