#include "10__probe_multitu_include_static_function_partition_runtime.h"

static int helper(void) {
    return 13;
}

int call_local_helper_from_header(void) {
    return helper();
}
