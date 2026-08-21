#include <stdio.h>
#include <stdlib.h>

#include "15__probe_axis15_wave1_runtime_clang_gcc16_tri_diff_multitu_flexible_array_packet_shared.h"

int main(void) {
    const size_t length = 9u;
    struct axis15_packet *packet = malloc(sizeof(*packet) + length);
    size_t i;
    unsigned first;
    unsigned second;

    if (!packet) {
        return 2;
    }
    packet->length = length;
    for (i = 0u; i < packet->length; ++i) {
        packet->bytes[i] = (unsigned char)(i * 19u + 7u);
    }
    first = axis15_digest(packet, 29u);
    packet->bytes[4] ^= 0x5au;
    second = axis15_digest(packet, 29u);
    printf("axis15-fam=%u,%u\n", first, second);
    free(packet);
    return 0;
}
