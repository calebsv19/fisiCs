#include <stdio.h>

#include "10__probe_multitu_include_internal_struct_twin_partition_runtime.h"

int main(void) {
    (void) bucket10_internal_header_struct_step_a(2, 1);
    (void) bucket10_internal_header_struct_step_b(-2, 3);
    printf("%d %d %d\n",
           bucket10_internal_header_struct_step_a(1, 2),
           bucket10_internal_header_struct_step_b(3, -1),
           bucket10_internal_header_struct_peek_a() + bucket10_internal_header_struct_peek_b());
    return 0;
}
