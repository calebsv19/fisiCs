#include <stdio.h>

#include "11__include_multitu_fnptr_table_route_runtime.h"

int main(void) {
    RouteFnInc first = fnptr_table_inc_pick(3);
    RouteFnInc second = fnptr_table_inc_pick(8);
    int total = first(7) + second(7);
    if (total != 25) return 1;
    printf("%d\n", total);
    return 0;
}
