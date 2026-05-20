#include <stdio.h>

typedef int (*ReseedRouteFn)(int, int *);

ReseedRouteFn fnptr_table_reseed_pick(int seed, int *cursor);

int main(void) {
    int cursor = 2;
    ReseedRouteFn first = fnptr_table_reseed_pick(3, &cursor);
    ReseedRouteFn second = fnptr_table_reseed_pick(4, &cursor);
    int total = first(5, &cursor) + second(5, &cursor);
    if (cursor != 8) return 1;
    if (total != 29) return 2;
    printf("%d %d\n", total, cursor);
    return 0;
}
