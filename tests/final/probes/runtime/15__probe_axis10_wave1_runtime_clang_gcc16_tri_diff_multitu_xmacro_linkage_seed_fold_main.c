#include <stdio.h>

#include "15__probe_axis10_wave1_runtime_clang_gcc16_tri_diff_multitu_xmacro_linkage_seed_fold_shared.h"

int main(void) {
    struct axis10_state state = {{0u, 0u, 0u, 0u}, 19u};
    unsigned i;

    for (i = 0u; i < 13u; ++i) {
        enum axis10_op op = (enum axis10_op)((i * 3u + 1u) % AXIS10_OP_COUNT);
        state = axis10_apply(state, op, i * 29u + 7u);
    }

    printf("axis10-xmacro=%u,%u,%u\n", state.digest, state.lanes[AXIS10_OP_advance],
           state.lanes[AXIS10_OP_finalize]);
    return 0;
}
