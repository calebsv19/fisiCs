#include <stdio.h>

typedef int (*map_fn)(int);

static int add3(int x) { return x + 3; }
static int twice(int x) { return x * 2; }

static int apply_chain(int value, map_fn *fns, int count) {
    int i;
    for (i = 0; i < count; ++i) {
        value = fns[i](value);
    }
    return value;
}

int main(void) {
    map_fn fns[3];
    fns[0] = add3;
    fns[1] = twice;
    fns[2] = add3;
    printf("%d\n", apply_chain(5, fns, 3));
    return 0;
}
