#include <stdio.h>

#include "11__include_multitu_fnptr_table_reseed_runtime.h"

int main(void) {
    int cursor = 1;
    ReseedRouteFnInc first = fnptr_table_inc_reseed_pick(2, &cursor);
    ReseedRouteFnInc second = fnptr_table_inc_reseed_pick(5, &cursor);
    int total = first(6, &cursor) + second(4, &cursor);
    if (cursor != 8) return 1;
    if (total != 33) return 2;
    printf("%d %d\n", total, cursor);
    return 0;
}
