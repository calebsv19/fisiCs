static const unsigned k_lrf_a_table[7] = {
    0x13579BDFu, 0x2468ACE1u, 0x10203040u, 0x55667788u,
    0x0BADF00Du, 0x31415926u, 0x89ABCDEFu
};

static unsigned g_lrf_a_state;
static unsigned g_lrf_a_round;

static unsigned rotl_u32(unsigned x, unsigned s) {
    unsigned r = s & 31u;
    if (r == 0u) {
        return x;
    }
    return (x << r) | (x >> (32u - r));
}

unsigned lrf_a_seed(void) {
    unsigned h = 2166136261u;
    unsigned i;
    for (i = 0u; i < 7u; ++i) {
        h ^= k_lrf_a_table[i] + i * 29u;
        h *= 16777619u;
    }
    return h;
}

void lrf_a_reset(unsigned salt) {
    g_lrf_a_state = lrf_a_seed() ^ (salt * 181u + 7u);
    g_lrf_a_round = 0u;
}

unsigned lrf_a_step(unsigned x, unsigned lane) {
    unsigned t = k_lrf_a_table[(lane + x + g_lrf_a_round) % 7u];
    ++g_lrf_a_round;
    g_lrf_a_state =
        rotl_u32(g_lrf_a_state ^ t ^ (lane * 11u + g_lrf_a_round), (lane & 5u) + 3u);
    return x + (g_lrf_a_state ^ (t >> 3));
}

unsigned lrf_a_feedback(void) {
    return g_lrf_a_state ^ (g_lrf_a_round * 97u);
}
