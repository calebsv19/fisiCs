#include <stdio.h>

typedef int (*RouteFn)(int);

RouteFn fnptr_table_pick(int seed);

int main(void) {
    RouteFn first = fnptr_table_pick(2);
    RouteFn second = fnptr_table_pick(5);
    int total = first(10) + second(10);
    if (total != 32) return 1;
    printf("%d\n", total);
    return 0;
}
