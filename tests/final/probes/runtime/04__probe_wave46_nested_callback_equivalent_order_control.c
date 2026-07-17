#include <stdio.h>

int wave46_nested_equivalent(int (*callback)(int value));
int wave46_nested_equivalent(int (*callback)());
int wave46_nested_equivalent(int (*callback)(int value));

int wave46_nested_array_equivalent(int (*callback)(int (*rows)[]));
int wave46_nested_array_equivalent(int (*callback)(int (*rows)[3]));
int wave46_nested_array_equivalent(int (*callback)(int (*rows)[3]));

static int wave46_add_one(int value) {
    return value + 1;
}

int wave46_nested_equivalent(int (*callback)(int value)) {
    return callback(45);
}

static int wave46_sum_row(int (*rows)[3]) {
    return (*rows)[0] + (*rows)[1] + (*rows)[2];
}

int wave46_nested_array_equivalent(int (*callback)(int (*rows)[3])) {
    int rows[1][3] = {{1, 2, 3}};
    return callback(rows);
}

int main(void) {
    int prototype_result = wave46_nested_equivalent(wave46_add_one);
    int array_result = wave46_nested_array_equivalent(wave46_sum_row);
    printf("%d\n", prototype_result + array_result);
    return 0;
}
