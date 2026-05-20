#include <stdio.h>

#include "10__probe_multitu_include_static_function_partition_runtime.h"

int helper(void) {
    return 26;
}

int main(void) {
    printf("%d %d\n", call_local_helper_from_header(), helper());
    return 0;
}
