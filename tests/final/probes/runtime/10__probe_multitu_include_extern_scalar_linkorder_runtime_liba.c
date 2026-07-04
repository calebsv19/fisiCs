#include "10__probe_multitu_include_extern_scalar_linkorder_runtime.h"

static int bucket10_linkorder_extern_lane;

void bucket10_linkorder_extern_reset_lane(int lane_seed) {
    bucket10_linkorder_extern_lane = lane_seed;
}

int bucket10_linkorder_extern_seed(int seed) {
    bucket10_linkorder_extern_lane = bucket10_linkorder_extern_scalar + seed * 2;
    bucket10_linkorder_extern_scalar += seed + 5;
    return bucket10_linkorder_extern_lane + bucket10_linkorder_extern_scalar;
}

int bucket10_linkorder_extern_add(int delta) {
    bucket10_linkorder_extern_lane += delta * 3 + 1;
    bucket10_linkorder_extern_scalar += bucket10_linkorder_extern_lane - delta;
    return bucket10_linkorder_extern_scalar - bucket10_linkorder_extern_lane;
}
