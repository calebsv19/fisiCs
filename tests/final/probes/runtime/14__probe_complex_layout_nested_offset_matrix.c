#include <stdio.h>
#include <stddef.h>

typedef struct {
    int lead;
    _Complex double pair;
} inner_layout;

typedef struct {
    char mark;
    inner_layout inner;
    _Complex double tail;
} outer_layout;

int main(void) {
    printf("%zu %zu %zu %zu\n",
           offsetof(outer_layout, inner),
           offsetof(outer_layout, inner.pair),
           offsetof(outer_layout, tail),
           sizeof(outer_layout));
    return 0;
}
