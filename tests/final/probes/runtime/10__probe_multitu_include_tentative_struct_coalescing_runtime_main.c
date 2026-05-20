#include <stdio.h>

#include "10__probe_multitu_include_tentative_struct_coalescing_runtime.h"

int main(void) {
    bucket10_header_struct_prime(5, 14);
    printf("%d %d %d\n",
           bucket10_header_struct_shift_left(4),
           bucket10_header_struct_shift_right(-2),
           bucket10_header_struct_total());
    return 0;
}
