#include <stdio.h>

#include "10__probe_multitu_include_extern_scalar_linkorder_runtime.h"

int bucket10_linkorder_extern_scalar = 19;

static int run_lane(int seed) {
    bucket10_linkorder_extern_scalar = 19 + seed;
    bucket10_linkorder_extern_reset_lane(seed * 3 + 1);
    bucket10_linkorder_extern_reset_bias(seed + 7);

    int first = bucket10_linkorder_extern_seed(seed);
    int second = bucket10_linkorder_extern_mix(seed + 4);
    int third = bucket10_linkorder_extern_add(seed - 1);
    int fourth = bucket10_linkorder_extern_mix(seed + 2);
    return first + second * 2 + third * 3 + fourth * 5 + bucket10_linkorder_extern_peek();
}

int main(void) {
    int r1 = run_lane(3);
    int r2 = run_lane(3);
    int r3 = run_lane(8);

    printf("%d %d %d %d\n", r1, r2, r3, bucket10_linkorder_extern_scalar);
    if (r1 != r2) {
        return 11;
    }
    return 0;
}
