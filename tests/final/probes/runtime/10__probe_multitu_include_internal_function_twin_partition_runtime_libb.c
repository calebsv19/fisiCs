#include "10__probe_multitu_include_internal_function_twin_partition_runtime.h"

static int bucket10_private_helper(int base) {
    return base * 2 + 5;
}

int bucket10_internal_header_step_b(int base) {
    return bucket10_private_helper(base) + bucket10_private_helper(1);
}
