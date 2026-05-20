#include "10__probe_multitu_include_tentative_header_array_current_strong_def_runtime.h"

void bucket10_header_prime(int seed) {
    int i;
    for (i = 0; i < 5; ++i) {
        bucket10_header_lane[i] = seed + i * 2;
    }
}

int bucket10_header_fold(int idx) {
    bucket10_header_lane[idx] += bucket10_header_lane[1] - bucket10_header_lane[0];
    bucket10_header_lane[0] += idx;
    return bucket10_header_lane[idx] + bucket10_header_lane[0];
}

int bucket10_header_total(void) {
    int total = 0;
    int i;
    for (i = 0; i < 5; ++i) {
        total += bucket10_header_lane[i];
    }
    return total;
}
