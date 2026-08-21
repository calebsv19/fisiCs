#include "15__probe_axis20_wave1_runtime_clang_gcc16_tri_diff_multitu_static_storage_initialization_shared.h"

unsigned axis20_explicit[3] = {17u, 31u, 47u};
unsigned axis20_zero;

unsigned axis20_step(unsigned seed) {
    axis20_zero = axis20_zero * 13u + seed + axis20_explicit[0];
    axis20_explicit[1] ^= axis20_zero + seed;
    axis20_explicit[2] += axis20_explicit[1] ^ (seed << 1u);
    return axis20_zero ^ axis20_explicit[1] ^ axis20_explicit[2];
}
