#include <stdio.h>

#include "10__probe_multitu_include_internal_struct_array_twin_partition_runtime.h"

int main(void) {
    (void) bucket10_internal_header_struct_array_step_a(0, 2);
    (void) bucket10_internal_header_struct_array_step_b(1, -1);
    printf("%d %d %d\n",
           bucket10_internal_header_struct_array_step_a(1, 3),
           bucket10_internal_header_struct_array_step_b(0, 4),
           bucket10_internal_header_struct_array_peek_a() + bucket10_internal_header_struct_array_peek_b());
    return 0;
}
