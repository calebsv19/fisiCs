extern int printf(const char*, ...);

#include "04__probe_multitu_include_param_array_extent_adjust_bridge_shared.h"

int main(void) {
    int values[4] = {10, 6, 3, 8};
    printf("%d\n", wave16_multitu_include_extent_fold(values));
    return 0;
}
