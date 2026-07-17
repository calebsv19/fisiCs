#include <stdio.h>

#include "11__probe_wave90_function_type_typedef_factory_contract.h"

static int wave90_add_two(int value) {
    return value + 2;
}

static int wave90_times_three(int value) {
    return value * 3;
}

static Wave90LeafFunction *wave90_choose_leaf(int route) {
    return route ? wave90_times_three : wave90_add_two;
}

int main(void) {
    int result = wave90_function_type_typedef_factory_call(
        wave90_add_two,
        wave90_choose_leaf,
        1,
        10);
    printf("%d\n", result);
    return 0;
}
