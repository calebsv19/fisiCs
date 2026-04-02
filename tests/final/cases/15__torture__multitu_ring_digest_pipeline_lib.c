unsigned mt13_aux_mix(unsigned x, unsigned y);

static unsigned g_ring[16];
static unsigned g_head;

unsigned mt13_ring_push(unsigned value) {
    unsigned slot = g_head & 15u;
    unsigned prev = g_ring[slot];
    g_ring[slot] = mt13_aux_mix(value, prev + slot * 13u);
    g_head++;
    return g_ring[slot];
}

unsigned mt13_ring_digest(void) {
    unsigned acc = 0x9e3779b9u;

    for (unsigned i = 0; i < 16u; ++i) {
        unsigned idx = (g_head + i) & 15u;
        acc = mt13_aux_mix(acc, g_ring[idx] + i * 19u);
    }

    return acc ^ g_head;
}
