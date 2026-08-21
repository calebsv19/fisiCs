#include "15__probe_axis9_wave1_runtime_clang_gcc16_tri_diff_multitu_complex_packet_seed_fold_shared.h"

/* All complex components are integral-valued and remain within the exact
 * integer range of double. `stamp` uses only defined unsigned arithmetic. */
struct axis9_packet axis9_step(struct axis9_packet input, unsigned seed) {
    struct axis9_packet output;
    double real_bias = (double)(seed & 7u);
    double imag_bias = (double)(input.stamp & 3u);

    output.value = input.value * (1.0 + 2.0 * I) + real_bias - imag_bias * I;
    output.stamp = input.stamp * 33u + seed * 17u + 5u;
    return output;
}
