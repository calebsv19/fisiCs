#include "15__probe_axis15_wave1_runtime_clang_gcc16_tri_diff_multitu_flexible_array_packet_shared.h"

unsigned axis15_digest(const struct axis15_packet *packet, unsigned seed) {
    size_t i;
    unsigned digest = seed;

    for (i = 0u; i < packet->length; ++i) {
        digest = (digest << 5u) ^ (digest >> 2u) ^ packet->bytes[i] ^ (unsigned)i;
    }
    return digest;
}
