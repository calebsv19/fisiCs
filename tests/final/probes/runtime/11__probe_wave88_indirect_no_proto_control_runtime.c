#include <stdio.h>

typedef int (*wave88_control_unprototyped_fn)();

static int wave88_control_callee(double value, int offset) {
    return (int)(value * 100.0) + offset;
}

static wave88_control_unprototyped_fn wave88_control_factory(void) {
    return wave88_control_callee;
}

int main(void) {
    wave88_control_unprototyped_fn direct = wave88_control_callee;
    int direct_result = direct(1.25, 7);
    int factory_result = wave88_control_factory()(2.50, 9);

    printf("%d %d\n", direct_result, factory_result);
    return (direct_result == 132 && factory_result == 259) ? 0 : 1;
}
