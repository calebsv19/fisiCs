#include "10__probe_multitu_include_static_struct_array_partition_runtime.h"

struct Bucket10HeaderArrayNode {
    int left;
    int right;
};

static struct Bucket10HeaderArrayNode lane[2] = {
    {2, 5},
    {6, 3},
};

int bucket10_header_local_struct_array_peek(void) {
    return lane[0].left + lane[0].right + lane[1].left + lane[1].right;
}

int bucket10_header_local_struct_array_step(int index, int delta) {
    lane[index].left += delta;
    lane[index].right += delta + index;
    return lane[0].left + lane[0].right + lane[1].left + lane[1].right;
}
