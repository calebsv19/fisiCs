#include <stdio.h>

#include "15__probe_axis12_wave1_runtime_clang_gcc16_tri_diff_multitu_static_inline_local_state_shared.h"

int main(void) {
    unsigned main_first = axis12_tick(7u);
    unsigned worker_first = axis12_worker(11u);
    unsigned main_second = axis12_tick(13u);
    unsigned worker_second = axis12_worker(17u);

    printf("axis12-inline=%u,%u,%u,%u\n", main_first, worker_first, main_second,
           worker_second);
    return 0;
}
