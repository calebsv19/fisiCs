#include "10__probe_multitu_include_static_struct_partition_runtime.h"

struct Bucket10HeaderStructState {
    int left;
    int right;
};

static struct Bucket10HeaderStructState lane = {5, 7};

int bucket10_header_local_struct_peek(void) {
    return lane.left + lane.right;
}

int bucket10_header_local_struct_step(int delta_left, int delta_right) {
    lane.left += delta_left;
    lane.right += delta_right;
    return lane.left + lane.right;
}
