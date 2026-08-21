#include "15__probe_axis12_wave1_runtime_clang_gcc16_tri_diff_multitu_static_inline_local_state_shared.h"

unsigned axis12_worker(unsigned seed) {
    unsigned first = axis12_tick(seed + 3u);
    unsigned second = axis12_tick(seed + 5u);
    return first * 31u + second;
}
