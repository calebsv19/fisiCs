#include <stdio.h>
#define OSP3_DEFAULT_SEED 0x715ee93du
#include "15__probe_osp3_policy_matrix_common.h"

static uint32_t transition(uint32_t state, uint32_t cancel,
                           uint32_t resources, uint32_t checkpoint) {
    if (state > 3u) return 7u;
    if (state == 2u || state == 3u) return state;
    if (cancel != 0u) return 3u;
    if (state == 1u && checkpoint == 0u) return 3u;
    if (resources == 0u) return 0u;
    return state == 0u ? 1u : 2u;
}

int main(void) {
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0x2d2816feu;
    uint32_t i;
    for (i = 0u; i < OSP3_CASE_BUDGET; ++i) {
        uint32_t bits = osp3_next(&seed);
        uint32_t state = bits & 7u;
        uint32_t cancel = (bits >> 3) & 1u;
        uint32_t resources = (bits >> 4) & 3u;
        uint32_t checkpoint = (bits >> 6) & 1u;
        uint32_t result = transition(state, cancel, resources, checkpoint);
        hash = osp3_mix(hash, result | (state << 8) | (i << 16));
    }
    printf("OSP3 queue seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
