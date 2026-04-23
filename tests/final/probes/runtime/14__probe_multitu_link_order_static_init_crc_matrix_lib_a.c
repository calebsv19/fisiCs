static const unsigned k_lo_a_table[8] = {
    0x13579BDFu, 0x2468ACE1u, 0x10203040u, 0x55667788u,
    0x0BADF00Du, 0xDEADBEEFu, 0x89ABCDEFu, 0x31415926u
};

static unsigned g_lo_a_state = 0xA11CE55Du;

static unsigned rotl_u32(unsigned x, unsigned s) {
    unsigned r = s & 31u;
    if (r == 0u) {
        return x;
    }
    return (x << r) | (x >> (32u - r));
}

unsigned lo_a_seed(void) {
    unsigned h = 2166136261u;
    unsigned i;
    for (i = 0u; i < 8u; ++i) {
        h ^= k_lo_a_table[i];
        h *= 16777619u;
    }
    return h;
}

void lo_a_reset(void) {
    g_lo_a_state = 0xA11CE55Du ^ k_lo_a_table[0];
}

unsigned lo_a_step(unsigned x, unsigned lane) {
    unsigned t = k_lo_a_table[(lane + x) & 7u];
    g_lo_a_state = rotl_u32(g_lo_a_state ^ t ^ (lane * 17u + 3u), (lane & 7u) + 1u);
    return (x + g_lo_a_state) ^ (t >> 3);
}

unsigned lo_a_state(void) {
    return g_lo_a_state;
}
