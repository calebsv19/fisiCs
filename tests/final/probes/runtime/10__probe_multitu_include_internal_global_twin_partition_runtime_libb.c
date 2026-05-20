#include "10__probe_multitu_include_internal_global_twin_partition_runtime.h"

static int bucket10_private_state = 20;

int bucket10_internal_header_global_step_b(int x) {
    bucket10_private_state = bucket10_private_state - x + 5;
    return bucket10_private_state;
}

int bucket10_internal_header_global_peek_b(void) {
    return bucket10_private_state;
}
