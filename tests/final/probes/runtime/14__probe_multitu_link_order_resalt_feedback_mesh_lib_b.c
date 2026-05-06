static const unsigned k_lrf_b_table[7] = {
    0xCAFEBABEu, 0xFACEB00Cu, 0x11223344u, 0x99AABBCCu,
    0x76543210u, 0x55AA55AAu, 0xDEADC0DEu
};

static unsigned g_lrf_b_state;
static unsigned g_lrf_b_round;

unsigned lrf_b_seed(void) {
    unsigned h = 2166136261u;
    unsigned i;
    for (i = 0u; i < 7u; ++i) {
        unsigned v = k_lrf_b_table[i] ^ (i * 131u + 17u);
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

void lrf_b_reset(unsigned salt) {
    g_lrf_b_state = lrf_b_seed() ^ (salt * 223u + 9u);
    g_lrf_b_round = 0u;
}

unsigned lrf_b_step(unsigned x, unsigned lane) {
    unsigned t = k_lrf_b_table[(lane * 5u + x + g_lrf_b_round) % 7u];
    ++g_lrf_b_round;
    g_lrf_b_state = (g_lrf_b_state * 131u) + (t ^ lane) + (g_lrf_b_round * 23u);
    return (x ^ g_lrf_b_state) + (t << 1);
}

unsigned lrf_b_feedback(void) {
    return g_lrf_b_state ^ (g_lrf_b_round * 131u);
}
