#include <stdio.h>

#include "10__probe_multitu_include_extern_array_static_local_shadow_current_runtime.h"

int bucket10_wave59_current_lane[4] = {1, 2, 3, 4};

static int bucket10_wave59_current_main_mix(int seed) {
    static int main_shadow_lane = 5;
    int shadow;

    main_shadow_lane += seed;
    shadow = main_shadow_lane;
    {
        int block_shadow_lane = shadow - seed + 2;
        shadow += block_shadow_lane;
    }
    return shadow;
}

int main(void) {
    int seeded = bucket10_wave59_current_lib_seed(3);
    int mixed = bucket10_wave59_current_main_mix(4);
    int folded = bucket10_wave59_current_lib_fold(2);
    int total = bucket10_wave59_current_lib_total();

    printf("%d %d %d %d\n", seeded, mixed, folded, total);
    return 0;
}
