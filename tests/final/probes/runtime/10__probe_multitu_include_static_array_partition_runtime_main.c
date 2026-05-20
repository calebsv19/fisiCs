#include <stdio.h>

#include "10__probe_multitu_include_static_array_partition_runtime.h"

int lane[3] = {4, 6, 8};

static int fold_external_lane(void) {
    return lane[0] + lane[1] + lane[2];
}

int main(void) {
    printf("%d %d ", bucket10_header_local_array_peek(), fold_external_lane());
    printf("%d %d\n",
           bucket10_header_local_array_step(1, 5),
           fold_external_lane());
    return 0;
}
