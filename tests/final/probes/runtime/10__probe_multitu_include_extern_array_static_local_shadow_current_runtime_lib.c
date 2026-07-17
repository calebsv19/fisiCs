#include "10__probe_multitu_include_extern_array_static_local_shadow_current_runtime.h"

int bucket10_wave59_current_lib_seed(int seed) {
    static int call_bias = 7;
    int shadow_lane = seed + call_bias;
    call_bias += seed;
    return shadow_lane;
}

int bucket10_wave59_current_lib_fold(int step) {
    static int fold_bias = 1;
    int sum = 0;
    int i;

    for (i = 0; i < 4; ++i) {
        bucket10_wave59_current_lane[i] += step + i + fold_bias;
        sum += bucket10_wave59_current_lane[i];
    }
    fold_bias += step;
    return sum;
}

int bucket10_wave59_current_lib_total(void) {
    return bucket10_wave59_current_lane[0] + bucket10_wave59_current_lane[1] +
           bucket10_wave59_current_lane[2] + bucket10_wave59_current_lane[3];
}
