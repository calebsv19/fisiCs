static const unsigned k_sief_b_boot[5] = {
    13u, 31u, 59u, 73u, 109u
};

static unsigned g_sief_b_state = 23u;
static unsigned g_sief_b_epoch = 0u;

static unsigned boot_hash_b(void) {
    unsigned h = 2166136261u;
    unsigned i;
    for (i = 0u; i < 5u; ++i) {
        unsigned v = k_sief_b_boot[i] ^ (i * 37u + 9u);
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

unsigned sief_b_boot(void) {
    return boot_hash_b();
}

void sief_b_reset(unsigned salt) {
    g_sief_b_state = boot_hash_b() ^ (salt * 241u + 5u);
    g_sief_b_epoch = salt % 19u;
}

unsigned sief_b_emit(unsigned lane) {
    unsigned tap = k_sief_b_boot[(lane * 3u) % 5u] + lane * 11u;
    g_sief_b_state = (g_sief_b_state * 131u) + tap + g_sief_b_epoch * 17u;
    ++g_sief_b_epoch;
    return (g_sief_b_state ^ tap) + (lane << 1);
}

unsigned sief_b_epoch(void) {
    return g_sief_b_state ^ (g_sief_b_epoch * 131u);
}
