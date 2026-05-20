#include <stdio.h>

#include "10__probe_multitu_include_tentative_header_array_runtime.h"

int main(void) {
    bucket10_header_prime(7);
    printf("%d %d %d\n",
           bucket10_header_lane[2],
           bucket10_header_fold(4),
           bucket10_header_total());
    return 0;
}
