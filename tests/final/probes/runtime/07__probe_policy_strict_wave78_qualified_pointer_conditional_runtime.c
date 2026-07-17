#include <stddef.h>
#include <stdio.h>

int main(void) {
    int values[5] = {11, 23, 37, 41, 59};
    int *mutable = values;
    const int *as_const = values + 1;
    volatile int *as_volatile = values + 2;
    const volatile int *selected = 1 ? as_const : as_volatile;
    const volatile void *opaque = 1 ? (const volatile void *)selected : (const volatile void *)0;
    const volatile int *roundtrip = (const volatile int *)opaque;
    ptrdiff_t offset = roundtrip - (const volatile int *)values;

    printf("%d %ld %d %d\n",
           *roundtrip,
           (long)offset,
           roundtrip == (const volatile int *)(mutable + 1),
           (0 ? (const volatile int *)0 : as_volatile) == (const volatile int *)(values + 2));
    return 0;
}
