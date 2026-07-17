#include <stdio.h>

int main(void) {
    int values[5] = {4, 9, 16, 25, 36};
    int *base = values;
    const int *mid = values + 2;
    const void *selected = (1 ? (0 ? (const void *)0 : (const void *)mid) : (const void *)(base + 4));
    int *roundtrip = (int *)(0 ? (void *)0 : (void *)(1 ? base + 3 : base));
    const int *nested_null = (0 ? (const int *)(values + 1) : (1 ? (const int *)0 : mid));
    const int *fallback = (nested_null ? nested_null : (const int *)selected);

    printf("%d %d %d %d\n",
           *(const int *)selected,
           *roundtrip,
           *fallback,
           selected != (const void *)0);
    return 0;
}
