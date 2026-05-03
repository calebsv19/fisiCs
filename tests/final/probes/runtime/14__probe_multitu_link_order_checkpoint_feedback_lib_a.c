static const unsigned k_lcf_a_table[6] = {
    0x13579BDFu, 0x2468ACE1u, 0x10203040u,
    0x55667788u, 0x0BADF00Du, 0x31415926u
};

static unsigned g_lcf_a_state;
static unsigned g_lcf_a_tick;

static unsigned rotl_u32(unsigned x, unsigned s) {
    unsigned r = s & 31u;
    if (r == 0u) {
        return x;
    }
    return (x << r) | (x >> (32u - r));
}

unsigned lcf_a_seed(void) {
    unsigned h = 2166136261u;
    unsigned i;
    for (i = 0u; i < 6u; ++i) {
        h ^= k_lcf_a_table[i] + i * 17u;
        h *= 16777619u;
    }
    return h;
}

void lcf_a_reset(unsigned salt) {
    g_lcf_a_state = lcf_a_seed() ^ (salt * 193u + 5u);
    g_lcf_a_tick = 0u;
}

unsigned lcf_a_step(unsigned x, unsigned lane) {
    unsigned t = k_lcf_a_table[(lane + x) % 6u];
    ++g_lcf_a_tick;
    g_lcf_a_state = rotl_u32(g_lcf_a_state ^ t ^ (lane * 13u + g_lcf_a_tick), (lane & 7u) + 1u);
    return x + (g_lcf_a_state ^ (t >> 2));
}

unsigned lcf_a_checkpoint(void) {
    return g_lcf_a_state ^ (g_lcf_a_tick * 97u);
}
