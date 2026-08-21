#include <stdio.h>

#include "15__probe_axis9_wave1_runtime_clang_gcc16_tri_diff_multitu_complex_packet_seed_fold_shared.h"

int main(void) {
    struct axis9_packet state;
    unsigned i;

    state.value = 3.0 + 4.0 * I;
    state.stamp = 11u;
    for (i = 0u; i < 6u; ++i) {
        state = axis9_step(state, i * 19u + 7u);
    }

    printf("axis9-complex=%ld,%ld,%u\n", (long)creal(state.value),
           (long)cimag(state.value), state.stamp);
    return 0;
}
