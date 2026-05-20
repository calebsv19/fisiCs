#include <stdio.h>

#include "10__probe_multitu_include_internal_global_twin_partition_runtime.h"

int main(void) {
    int first;
    int second;
    int total;

    (void) bucket10_internal_header_global_step_a(4);
    (void) bucket10_internal_header_global_step_b(6);
    first = bucket10_internal_header_global_step_a(1);
    second = bucket10_internal_header_global_step_b(2);
    total = bucket10_internal_header_global_peek_a() + bucket10_internal_header_global_peek_b();
    printf("%d %d %d\n", first, second, total);
    return 0;
}
