#include <stdio.h>
#define OSP3_DEFAULT_SEED 0xf1357bd9u
#include "15__probe_osp3_policy_matrix_common.h"

struct checkpoint {
    uint32_t epoch;
    uint32_t owner;
    uint32_t lanes[4];
};

static struct checkpoint mutate(struct checkpoint input, uint32_t bits) {
    uint32_t lane = bits & 3u;
    input.epoch += 1u;
    input.owner = (input.owner + ((bits >> 2) & 7u)) & 15u;
    input.lanes[lane] ^= bits;
    input.lanes[(lane + 1u) & 3u] += input.epoch;
    return input;
}

int main(void) {
    struct checkpoint state = {0u, 1u, {3u, 5u, 7u, 11u}};
    uint32_t seed = OSP3_SEED;
    uint32_t hash = 0xcd9e8d57u;
    uint32_t i;
    for (i = 0u; i < OSP3_CASE_BUDGET; ++i) {
        uint32_t lane;
        state = mutate(state, osp3_next(&seed));
        hash = osp3_mix(hash, state.epoch ^ (state.owner << 16));
        for (lane = 0u; lane < 4u; ++lane) {
            hash = osp3_mix(hash, state.lanes[lane]);
        }
    }
    printf("OSP3 aggregate seed=%08x cases=%u digest=%u\n",
           (unsigned)OSP3_SEED, (unsigned)OSP3_CASE_BUDGET, (unsigned)hash);
    return 0;
}
