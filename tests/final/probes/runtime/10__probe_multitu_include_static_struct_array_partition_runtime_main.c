#include <stdio.h>

#include "10__probe_multitu_include_static_struct_array_partition_runtime.h"

struct Bucket10HeaderArrayNode {
    int left;
    int right;
};

struct Bucket10HeaderArrayNode lane[2] = {
    {4, 1},
    {7, 2},
};

static int fold_external_lane(void) {
    return lane[0].left + lane[0].right + lane[1].left + lane[1].right;
}

int main(void) {
    printf("%d %d ", bucket10_header_local_struct_array_peek(), fold_external_lane());
    printf("%d %d\n",
           bucket10_header_local_struct_array_step(1, 3),
           fold_external_lane());
    return 0;
}
