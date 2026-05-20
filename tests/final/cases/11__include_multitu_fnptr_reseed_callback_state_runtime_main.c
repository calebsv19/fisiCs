#include <stdio.h>

#include "11__include_multitu_fnptr_reseed_callback_state_runtime.h"

int main(void) {
    int cursor = 1;
    int total = 0;
    total += fnptr_inc_reseed_state_pick(2, &cursor)(&cursor, 3);
    total += fnptr_inc_reseed_state_pick(5, &cursor)(&cursor, 1);
    if (cursor != 8) return 1;
    if (total != 17) return 2;
    printf("%d %d\n", total, cursor);
    return 0;
}
