#include <stdio.h>

int main(void) {
    int values[6] = {3, 5, 8, 13, 21, 34};
    int *head = values;
    int *tail = values + 3;
    const int *selected = (1 ? tail + 1 : (const int *)0);
    const void *as_void = (0 ? (const void *)head : (const void *)selected);
    int *roundtrip = (int *)(1 ? (void *)(tail + 2) : (void *)0);
    long span = (long)((1 ? tail + 2 : head) - (0 ? tail : head + 1));

    printf("%d %d %d %ld %d\n",
           *selected,
           *(const int *)as_void,
           *roundtrip,
           span,
           selected == values + 4);
    return 0;
}
