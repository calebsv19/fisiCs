#include <stdio.h>

#include "10__probe_multitu_include_internal_function_twin_partition_runtime.h"

int main(void) {
    printf("%d %d %d\n",
           bucket10_internal_header_step_a(3),
           bucket10_internal_header_step_b(3),
           bucket10_internal_header_step_a(0) + bucket10_internal_header_step_b(0));
    return 0;
}
