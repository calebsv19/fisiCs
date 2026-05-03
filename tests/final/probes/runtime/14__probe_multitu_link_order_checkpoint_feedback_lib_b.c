static const unsigned k_lcf_b_table[6] = {
    0xCAFEBABEu, 0xFACEB00Cu, 0x11223344u,
    0x99AABBCCu, 0x76543210u, 0x55AA55AAu
};

static unsigned g_lcf_b_state;
static unsigned g_lcf_b_tick;

unsigned lcf_b_seed(void) {
    unsigned h = 2166136261u;
    unsigned i;
    for (i = 0u; i < 6u; ++i) {
        unsigned v = k_lcf_b_table[i] ^ (i * 131u + 11u);
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

void lcf_b_reset(unsigned salt) {
    g_lcf_b_state = lcf_b_seed() ^ (salt * 257u + 9u);
    g_lcf_b_tick = 0u;
}

unsigned lcf_b_step(unsigned x, unsigned lane) {
    unsigned t = k_lcf_b_table[(lane * 3u + x) % 6u];
    ++g_lcf_b_tick;
    g_lcf_b_state = (g_lcf_b_state * 131u) + (t ^ lane) + (g_lcf_b_tick * 17u);
    return (x ^ g_lcf_b_state) + (t << 1);
}

unsigned lcf_b_checkpoint(void) {
    return g_lcf_b_state ^ (g_lcf_b_tick * 131u);
}
