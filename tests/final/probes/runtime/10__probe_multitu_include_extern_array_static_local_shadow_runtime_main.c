#include <stdio.h>

#include "10__probe_multitu_include_extern_array_static_local_shadow_runtime.h"

int bucket10_wave59_lane[4] = {1, 2, 3, 4};

static int bucket10_wave59_main_mix(int seed) {
    static int bucket10_wave59_lane = 5;
    int shadow;

    bucket10_wave59_lane += seed;
    shadow = bucket10_wave59_lane;
    {
        int bucket10_wave59_lane = shadow - seed + 2;
        shadow += bucket10_wave59_lane;
    }
    return shadow;
}

int main(void) {
    int seeded = bucket10_wave59_lib_seed(3);
    int mixed = bucket10_wave59_main_mix(4);
    int folded = bucket10_wave59_lib_fold(2);
    int total = bucket10_wave59_lib_total();

    printf("%d %d %d %d\n", seeded, mixed, folded, total);
    return 0;
}
