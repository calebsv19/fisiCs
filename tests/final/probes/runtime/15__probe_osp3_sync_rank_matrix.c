#include <stdio.h>
#define OSP3_DEFAULT_SEED 0x4a4f92c1u
#include "15__probe_osp3_policy_matrix_common.h"

static uint32_t rank_policy(uint32_t held, uint32_t requested) {
    if (requested == 0u || requested > 4u) return 1u;
    if (held > 4u) return 2u;
    if (held != 0u && requested <= held) return 3u;
    return 0u;
}

int main(void) {
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0x7f4a7c15u;
    uint32_t i;
    for (i = 0u; i < OSP3_CASE_BUDGET; ++i) {
        uint32_t bits = osp3_next(&seed);
        uint32_t held = bits & 7u;
        uint32_t requested = (bits >> 3) & 7u;
        uint32_t result = rank_policy(held, requested);
        hash = osp3_mix(hash, result | (held << 4) | (requested << 8));
    }
    printf("OSP3 sync-rank seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
