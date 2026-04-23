static const unsigned k_lo_b_table[8] = {
    0xCAFEBABEu, 0xFACEB00Cu, 0x11223344u, 0x99AABBCCu,
    0x76543210u, 0x02468ACEu, 0xF0E1D2C3u, 0x55AA55AAu
};

static unsigned g_lo_b_state = 0xB01DFACEu;

unsigned lo_b_seed(void) {
    unsigned h = 2166136261u;
    unsigned i;
    for (i = 0u; i < 8u; ++i) {
        unsigned v = k_lo_b_table[i] ^ (i * 131u + 11u);
        h ^= (v & 0xFFu);
        h *= 16777619u;
        h ^= ((v >> 8) & 0xFFu);
        h *= 16777619u;
        h ^= ((v >> 16) & 0xFFu);
        h *= 16777619u;
        h ^= ((v >> 24) & 0xFFu);
        h *= 16777619u;
    }
    return h;
}

void lo_b_reset(void) {
    g_lo_b_state = 0xB01DFACEu ^ k_lo_b_table[1];
}

unsigned lo_b_step(unsigned x, unsigned lane) {
    unsigned t = k_lo_b_table[(lane * 3u + x) & 7u];
    g_lo_b_state = (g_lo_b_state * 131u) + (t ^ lane) + 0x9E37u;
    return (x ^ g_lo_b_state) + (t << 1);
}

unsigned lo_b_state(void) {
    return g_lo_b_state;
}
