#include <stdio.h>

static int add2(int x) {
    return x + 2;
}

static int mul3(int x) {
    return x * 3;
}

static int apply(int (*fn)(int), int *row, int offset) {
    return fn(row[offset]);
}

int main(void) {
    int left[3] = {2, 4, 6};
    int right[3] = {3, 6, 9};

    int *row0 = (1 ? left : right);
    int *row1 = (0 ? left : right);
    int (*fn0)(int) = (1 ? add2 : mul3);

    int value0 = apply(fn0, row0, 2);
    int value1 = apply((0 ? add2 : mul3), row1, 1);
    int ok_fn_nonnull = ((1 ? add2 : 0) != 0);
    int ok_fn_null = ((0 ? add2 : 0) == 0);

    printf("%d %d %d %d\n", value0, value1, ok_fn_nonnull, ok_fn_null);
    return 0;
}
