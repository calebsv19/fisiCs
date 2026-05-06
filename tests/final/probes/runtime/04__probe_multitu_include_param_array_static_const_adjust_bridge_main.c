extern int printf(const char*, ...);

#include "04__probe_multitu_include_param_array_static_const_adjust_bridge_shared.h"

int main(void) {
    int values[5] = {5, 1, 4, 2, 3};
    printf("%d\n", wave17_multitu_include_static_const_fold(values));
    return 0;
}
