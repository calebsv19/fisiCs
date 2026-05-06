static const unsigned k_sief_a_boot[5] = {
    11u, 29u, 47u, 71u, 101u
};

static unsigned g_sief_a_state = 19u;
static unsigned g_sief_a_epoch = 0u;

static unsigned boot_hash(void) {
    unsigned h = 2166136261u;
    unsigned i;
    for (i = 0u; i < 5u; ++i) {
        h ^= k_sief_a_boot[i] + i * 23u;
        h *= 16777619u;
    }
    return h;
}

unsigned sief_a_boot(void) {
    return boot_hash();
}

void sief_a_reset(unsigned salt) {
    g_sief_a_state = boot_hash() ^ (salt * 211u + 3u);
    g_sief_a_epoch = salt % 17u;
}

unsigned sief_a_emit(unsigned lane) {
    unsigned tap = k_sief_a_boot[lane % 5u] * (lane + 5u);
    g_sief_a_state = (g_sief_a_state + tap + g_sief_a_epoch * 13u) ^ (lane * 19u + 7u);
    ++g_sief_a_epoch;
    return g_sief_a_state ^ (tap >> 1);
}

unsigned sief_a_epoch(void) {
    return g_sief_a_state ^ (g_sief_a_epoch * 97u);
}
