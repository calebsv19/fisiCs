#include <stdio.h>

#include "15__probe_axis11_wave1_runtime_clang_gcc16_tri_diff_multitu_uint64_rotate_packet_shared.h"

int main(void) {
    struct axis11_packet state;
    unsigned i;

    state.value = UINT64_C(0x0123456789abcdef);
    state.tag = UINT32_C(19);
    for (i = 0u; i < 11u; ++i) {
        uint64_t seed = UINT64_C(0x1020304050607080) + (uint64_t)i * UINT64_C(0x101);
        state = axis11_round(state, seed, i * 5u + 3u);
    }

    printf("axis11-u64=%llu,%u\n", (unsigned long long)state.value,
           (unsigned)state.tag);
    return 0;
}
