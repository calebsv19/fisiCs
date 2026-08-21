#include "15__probe_axis10_wave1_runtime_clang_gcc16_tri_diff_multitu_xmacro_linkage_seed_fold_shared.h"

struct axis10_state axis10_apply(struct axis10_state input, enum axis10_op op,
                                 unsigned seed) {
    unsigned weight = 0u;

    switch (op) {
#define AXIS10_CASE(name, value) \
    case AXIS10_OP_##name: weight = (value); break;
        AXIS10_OPS(AXIS10_CASE)
#undef AXIS10_CASE
    default:
        return input;
    }

    input.lanes[op] = input.lanes[op] * 17u + AXIS10_SEED(seed, weight);
    input.digest = (input.digest << 5u) ^ input.lanes[op] ^ (weight * 97u);
    return input;
}
