#include "10__probe_multitu_include_internal_struct_array_twin_partition_runtime.h"

struct Bucket10HeaderTwinArrayNode {
    int left;
    int right;
};

static struct Bucket10HeaderTwinArrayNode bucket10_private_lane[2] = {
    {1, 2},
    {3, 4},
};

int bucket10_internal_header_struct_array_step_a(int index, int delta) {
    bucket10_private_lane[index].left += delta;
    bucket10_private_lane[index].right += delta + index;
    return bucket10_private_lane[0].left + bucket10_private_lane[0].right +
           bucket10_private_lane[1].left + bucket10_private_lane[1].right;
}

int bucket10_internal_header_struct_array_peek_a(void) {
    return bucket10_private_lane[0].left + bucket10_private_lane[0].right +
           bucket10_private_lane[1].left + bucket10_private_lane[1].right;
}
