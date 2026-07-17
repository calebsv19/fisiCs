#include <stdio.h>

static int inc(int value) {
    return value + 1;
}

static int triple(int value) {
    return value * 3;
}

static int call_selected(int (*fn)(int), int *row, int index) {
    return fn(row[index]);
}

int main(void) {
    int left[4] = {1, 2, 3, 4};
    int right[4] = {5, 6, 7, 8};
    int (*ops[2])(int) = {inc, triple};

    int *row = (0 ? left : right);
    int (*fn)(int) = (1 ? ops[1] : inc);
    int (*fallback)(int) = (0 ? ops[0] : triple);
    int call0 = call_selected(fn, row, 2);
    int call1 = call_selected(fallback, (1 ? left : right), 3);
    int nonnull = (1 ? inc : 0) != 0;

    printf("%d %d %d %d\n",
           call0,
           call1,
           nonnull,
           (0 ? left : right) == right);
    return 0;
}
