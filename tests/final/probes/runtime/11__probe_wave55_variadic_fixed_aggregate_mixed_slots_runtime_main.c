#include <stdio.h>

struct wave55_fixed_prefix {
    long stamp;
    int lane[2];
};

struct wave55_var_item {
    int left;
    int right;
};

long wave55_variadic_fixed_aggregate_mixed_slots(struct wave55_fixed_prefix prefix, int count, ...);

int main(void) {
    struct wave55_fixed_prefix prefix = {17, {3, 8}};
    struct wave55_var_item a = {2, 5};
    struct wave55_var_item b = {7, 4};
    struct wave55_var_item c = {1, 9};

    printf("%ld\n", wave55_variadic_fixed_aggregate_mixed_slots(
        prefix, 3, a, 6, b, -2, c, 11));
    return 0;
}
