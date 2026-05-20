#include <stdio.h>

#include "11__include_multitu_fnptr_callback_accumulate_runtime.h"

static int callback_inc_mix(int seed, int step) {
    return seed * 2 + step;
}

int main(void) {
    int total = fnptr_callback_inc_accumulate(4, 5, callback_inc_mix);
    if (total != 50) return 1;
    printf("%d\n", total);
    return 0;
}
