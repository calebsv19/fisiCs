#include <stdio.h>

static int add2(int x) {
    return x + 2;
}

static int mul4(int x) {
    return x * 4;
}

static int apply(int (*fn)(int), int *row, int index) {
    return fn(row[index]);
}

int main(void) {
    int left[3] = {1, 3, 5};
    int right[3] = {2, 4, 6};
    int (*table[2])(int) = {add2, mul4};

    int *row0 = (1 ? left : right);
    int *row1 = (0 ? left : right);
    int (*fn0)(int) = (1 ? add2 : mul4);
    int (*fn1)(int) = (0 ? add2 : table[1]);

    int direct = apply(fn0, row0, 2);
    int indirect = apply(fn1, row1, 1);
    int ok_fn_decay = ((1 ? add2 : 0) != 0);
    int ok_array_decay = ((0 ? left : right) == right);

    printf("%d %d %d %d\n", direct, indirect, ok_fn_decay, ok_array_decay);
    return 0;
}
