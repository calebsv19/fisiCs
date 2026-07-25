#include <stdio.h>
#define OSP3_DEFAULT_SEED 0xb5297a4du
#include "15__probe_osp3_policy_matrix_common.h"

static uint64_t make_token(uint32_t generation, uint32_t index) {
    return ((uint64_t)generation << 32) | (uint64_t)index;
}

static uint32_t validate_token(uint64_t token, uint32_t generation,
                               uint32_t index, uint32_t live) {
    if (live == 0u) return 1u;
    if ((uint32_t)(token >> 32) != generation) return 2u;
    if ((uint32_t)token != index) return 3u;
    return 0u;
}

int main(void) {
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0x68e31da4u;
    uint32_t i;
    for (i = 0u; i < OSP3_CASE_BUDGET; ++i) {
        uint32_t generation = osp3_next(&seed);
        uint32_t index = osp3_next(&seed) & 255u;
        uint64_t token = make_token(generation, index);
        uint32_t mutate = osp3_next(&seed);
        uint32_t observed_generation = generation + ((mutate & 1u) != 0u);
        uint32_t observed_index = index ^ ((mutate >> 1) & 1u);
        uint32_t live = (mutate >> 2) & 1u;
        hash = osp3_mix(hash, validate_token(token, observed_generation,
                                            observed_index, live));
        hash = osp3_mix(hash, generation ^ index);
    }
    printf("OSP3 token seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
