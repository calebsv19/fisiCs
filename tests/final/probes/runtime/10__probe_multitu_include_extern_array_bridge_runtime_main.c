#include <stdio.h>

#include "10__probe_multitu_include_extern_array_bridge_runtime.h"

int values[4] = {1, 2, 3, 4};

int main(void) {
    printf("%d ", accumulate_values());
    tweak_value(1, 5);
    printf("%d %d\n", accumulate_values(), values[1]);
    return 0;
}
