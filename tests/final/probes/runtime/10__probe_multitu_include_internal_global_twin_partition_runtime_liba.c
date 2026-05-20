#include "10__probe_multitu_include_internal_global_twin_partition_runtime.h"

static int bucket10_private_state = 3;

int bucket10_internal_header_global_step_a(int x) {
    bucket10_private_state = bucket10_private_state * 2 + x;
    return bucket10_private_state;
}

int bucket10_internal_header_global_peek_a(void) {
    return bucket10_private_state;
}
