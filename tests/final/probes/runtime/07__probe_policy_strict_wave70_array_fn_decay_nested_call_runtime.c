#include <stdio.h>

static int add_index(int *row, int index) {
    return row[index] + index;
}

static int scale_index(int *row, int index) {
    return row[index] * (index + 1);
}

static int invoke(int (*fn)(int *, int), int row[4], int index) {
    return fn(row, index);
}

int main(void) {
    int left[4] = {2, 4, 6, 8};
    int right[4] = {3, 5, 7, 9};
    int (*ops[2])(int *, int) = {add_index, scale_index};

    int *selected_row = (1 ? (0 ? left : right) : left);
    int (*selected_fn)(int *, int) = (0 ? ops[0] : (1 ? scale_index : add_index));
    int first = invoke(selected_fn, selected_row, 2);
    int second = (1 ? ops[0] : ops[1])((0 ? right : left), 3);
    int row_match = (0 ? left : (1 ? right : left)) == right;

    printf("%d %d %d %d\n",
           first,
           second,
           row_match,
           (selected_fn != 0));
    return 0;
}
