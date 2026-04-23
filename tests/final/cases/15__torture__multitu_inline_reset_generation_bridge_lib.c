#include "15__bridge_inline_reset_generation_shared.h"

static unsigned g_generation[3];
static unsigned g_value[3];

void bridge15_init(unsigned seed) {
    unsigned i;
    for (i = 0u; i < 3u; ++i) {
        bridge15_reset(&g_generation[i], seed + i * 3u);
        g_value[i] = bridge15_scramble(seed + i * 11u);
    }
}

unsigned bridge15_step(unsigned lane, unsigned payload) {
    unsigned idx = lane % 3u;
    g_generation[idx] = bridge15_scramble(g_generation[idx] + payload + idx * 13u);
    g_value[idx] ^= bridge15_scramble(payload + g_generation[idx]);
    return g_value[idx] ^ g_generation[idx];
}

unsigned bridge15_digest(void) {
    unsigned i;
    unsigned digest = 0x13579bdu;
    for (i = 0u; i < 3u; ++i) {
        digest ^= bridge15_scramble(g_generation[i] + i * 17u);
        digest = bridge15_scramble(digest + g_value[i] + i * 31u);
    }
    return digest;
}
