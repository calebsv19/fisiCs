#include <stdio.h>

#include "10__probe_multitu_include_static_struct_partition_runtime.h"

struct Bucket10HeaderStructState {
    int left;
    int right;
};

struct Bucket10HeaderStructState lane = {8, 3};

int main(void) {
    printf("%d %d ", bucket10_header_local_struct_peek(), lane.left + lane.right);
    printf("%d %d\n",
           bucket10_header_local_struct_step(2, 4),
           lane.left + lane.right);
    return 0;
}
