#include <stdio.h>

#include "11__include_multitu_fnptr_return_dispatch_runtime.h"

int main(void) {
    ProbeIntFnInc a = probe_pick_transform_inc(0);
    ProbeIntFnInc b = probe_pick_transform_inc(1);
    int total = a(6) + b(2);
    if (total != 19) return 1;
    printf("%d\n", total);
    return 0;
}
