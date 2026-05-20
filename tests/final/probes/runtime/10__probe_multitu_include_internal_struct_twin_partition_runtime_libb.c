#include "10__probe_multitu_include_internal_struct_twin_partition_runtime.h"

struct Bucket10HeaderTwinStruct {
    int left;
    int right;
};

static struct Bucket10HeaderTwinStruct bucket10_private_struct = {7, 3};

int bucket10_internal_header_struct_step_b(int delta_left, int delta_right) {
    bucket10_private_struct.left += delta_left;
    bucket10_private_struct.right += delta_right;
    return bucket10_private_struct.left + bucket10_private_struct.right;
}

int bucket10_internal_header_struct_peek_b(void) {
    return bucket10_private_struct.left + bucket10_private_struct.right;
}
