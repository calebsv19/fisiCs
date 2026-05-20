#include "10__probe_multitu_include_static_global_partition_runtime.h"

static int state = 40;

int read_local_state_from_header(void) {
    return state;
}

void bump_local_state_from_header(int delta) {
    state += delta;
}
