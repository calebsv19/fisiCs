#include <stddef.h>
#include <stdio.h>

struct View {
    const int *primary;
    const int *secondary;
    const int *fallback;
};

int main(void) {
    int values[5] = {13, 29, 43, 61, 71};
    struct View view = {values, values + 2, values + 4};
    int first = 0;
    int second = 1;
    const int *selected = first ? view.primary : (second ? view.secondary : view.fallback);
    const void *opaque = second ? (const void *)selected : (const void *)0;
    const int *roundtrip = (const int *)opaque;

    printf("%d %ld %d\n", *roundtrip, (long)(roundtrip - values), roundtrip == values + 2);
    return 0;
}
