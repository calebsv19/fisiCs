#include <stdio.h>

#include "10__probe_multitu_include_tentative_header_array_current_strong_def_runtime.h"

int bucket10_header_lane[5] = {0, 0, 0, 0, 0};

int main(void) {
    int third;
    int folded;
    int total;

    bucket10_header_prime(7);
    third = bucket10_header_lane[2];
    folded = bucket10_header_fold(4);
    total = bucket10_header_total();
    printf("%d %d %d\n", third, folded, total);
    return 0;
}
