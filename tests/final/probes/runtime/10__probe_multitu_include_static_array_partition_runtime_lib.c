#include "10__probe_multitu_include_static_array_partition_runtime.h"

static int lane[3] = {1, 3, 5};

int bucket10_header_local_array_peek(void) {
    return lane[0] + lane[1] + lane[2];
}

int bucket10_header_local_array_step(int index, int delta) {
    lane[index] += delta;
    return lane[0] + lane[1] + lane[2];
}
