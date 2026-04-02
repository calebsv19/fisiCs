unsigned mt15_mix(unsigned x, unsigned y);

static unsigned g_ring[16];
static unsigned g_head = 0u;
static unsigned g_state = 0x9e3779b9u;

unsigned mt15_reseed(unsigned seed) {
    g_head = 0u;
    g_state = mt15_mix(seed ^ 0xa5a5a5a5u, seed + 0x7f4a7c15u);
    for (unsigned i = 0; i < 16u; ++i) {
        g_ring[i] = mt15_mix(seed + i * 109u, g_state ^ (i * 53u));
    }
    return g_state;
}

unsigned mt15_emit(unsigned lane, unsigned input) {
    unsigned idx = (g_head + lane) & 15u;
    unsigned mixed = mt15_mix(input + g_state + lane * 17u, g_ring[idx] ^ (lane * 131u));
    g_ring[idx] = mixed;
    g_head = (g_head + 1u) & 15u;
    g_state = mt15_mix(g_state + mixed, lane + idx * 7u);
    return g_state ^ mixed;
}

unsigned mt15_checksum(void) {
    unsigned acc = g_state;
    for (unsigned i = 0; i < 16u; ++i) {
        unsigned idx = (g_head + i) & 15u;
        acc = mt15_mix(acc, g_ring[idx] + i * 19u);
    }
    return acc ^ g_head;
}
