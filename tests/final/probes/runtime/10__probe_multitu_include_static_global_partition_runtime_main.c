#include <stdio.h>

#include "10__probe_multitu_include_static_global_partition_runtime.h"

int state = 9;

int main(void) {
    printf("%d %d ", read_local_state_from_header(), state);
    bump_local_state_from_header(6);
    printf("%d %d\n", read_local_state_from_header(), state);
    return 0;
}
