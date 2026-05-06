extern int printf(const char*, ...);

#include "04__probe_multitu_include_param_array_restrict_adjust_bridge_shared.h"

int main(void) {
    int values[4] = {9, 2, 8, 7};
    printf("%d\n", wave17_multitu_include_restrict_fold(values));
    return 0;
}
