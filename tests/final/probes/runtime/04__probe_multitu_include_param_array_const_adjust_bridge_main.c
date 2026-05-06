extern int printf(const char*, ...);

#include "04__probe_multitu_include_param_array_const_adjust_bridge_shared.h"

int main(void) {
    int values[5] = {8, 3, 14, 6, 5};
    printf("%d\n", wave16_multitu_include_const_fold(values));
    return 0;
}
