#include "10__probe_multitu_include_extern_scalar_linkorder_runtime.h"

static int bucket10_linkorder_extern_bias = 7;

void bucket10_linkorder_extern_reset_bias(int bias_seed) {
    bucket10_linkorder_extern_bias = bias_seed;
}

int bucket10_linkorder_extern_mix(int seed) {
    bucket10_linkorder_extern_bias = bucket10_linkorder_extern_bias * 2 + seed + (bucket10_linkorder_extern_scalar % 6);
    bucket10_linkorder_extern_scalar += bucket10_linkorder_extern_bias - seed;
    return bucket10_linkorder_extern_scalar + bucket10_linkorder_extern_bias;
}

int bucket10_linkorder_extern_peek(void) {
    return bucket10_linkorder_extern_scalar * 2 - bucket10_linkorder_extern_bias;
}
