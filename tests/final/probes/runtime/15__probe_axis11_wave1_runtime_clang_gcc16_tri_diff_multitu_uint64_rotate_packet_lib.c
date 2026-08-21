#include "15__probe_axis11_wave1_runtime_clang_gcc16_tri_diff_multitu_uint64_rotate_packet_shared.h"

static uint64_t axis11_rotl(uint64_t value, unsigned amount) {
    return (value << amount) | (value >> (64u - amount));
}

struct axis11_packet axis11_round(struct axis11_packet input, uint64_t seed,
                                  unsigned lane) {
    unsigned amount = (lane % 31u) + 1u;
    struct axis11_packet output;

    output.value = axis11_rotl(input.value ^ seed, amount) + UINT64_C(0x9e3779b97f4a7c15);
    output.tag = input.tag * UINT32_C(65599) + (uint32_t)(seed >> 32u) + lane;
    return output;
}
