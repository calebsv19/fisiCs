#include <stdio.h>
#define OSP3_DEFAULT_SEED 0xe87a3c55u
#include "15__probe_osp3_policy_matrix_common.h"

static uint32_t extent_policy(uint32_t start, uint32_t count,
                              uint32_t limit, uint32_t owner) {
    if (owner == 0u) return 1u;
    if (count == 0u) return 2u;
    if (start >= limit) return 3u;
    if (count > limit - start) return 4u;
    if ((start & 3u) != 0u) return 5u;
    return 0u;
}

int main(void) {
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0x165667b1u;
    uint32_t i;
    for (i = 0u; i < OSP3_CASE_BUDGET; ++i) {
        uint32_t bits = osp3_next(&seed);
        uint32_t start = bits & 1023u;
        uint32_t count = osp3_next(&seed) & 511u;
        uint32_t limit = 256u + ((bits >> 10) & 1023u);
        uint32_t owner = (bits >> 21) & 7u;
        if ((i & 31u) == 0u) start = 0xfffffff0u + (i & 15u);
        hash = osp3_mix(hash, extent_policy(start, count, limit, owner));
        hash = osp3_mix(hash, start ^ count ^ limit);
    }
    printf("OSP3 extent seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
