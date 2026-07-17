#include <stdio.h>

struct wave78_fixed {
    long seed;
    int lane;
};

struct wave78_pair {
    int a;
    int b;
};

typedef long (*wave78_variadic_fn)(struct wave78_fixed fixed, int count, ...);

wave78_variadic_fn wave78_factory(int mode);

int main(void) {
    struct wave78_fixed fixed = {11, 4};
    struct wave78_pair first = {2, 3};
    struct wave78_pair second = {5, 7};
    wave78_variadic_fn sum = wave78_factory(0);
    wave78_variadic_fn alternate = wave78_factory(1);
    long a = sum(fixed, 2, first, second);
    long b = alternate(fixed, 2, first, second);

    printf("%ld %ld\n", a, b);
    return 0;
}
