#define OWN14_LANES 6u

static unsigned g_generation[OWN14_LANES];
static unsigned g_release_count[OWN14_LANES];
static int g_active[OWN14_LANES];

void own14_reset(void) {
    unsigned i;
    for (i = 0u; i < OWN14_LANES; ++i) {
        g_generation[i] = 0u;
        g_release_count[i] = 0u;
        g_active[i] = 0;
    }
}

void own14_set(unsigned lane, int enabled) {
    unsigned idx = lane % OWN14_LANES;

    if (enabled) {
        if (!g_active[idx]) {
            g_active[idx] = 1;
            g_generation[idx] += 1u;
        }
        return;
    }

    if (g_active[idx]) {
        g_active[idx] = 0;
        g_release_count[idx] += 1u;
    }
}

unsigned own14_release_total(void) {
    unsigned i;
    unsigned total = 0u;
    for (i = 0u; i < OWN14_LANES; ++i) {
        total += g_release_count[i];
    }
    return total;
}

unsigned own14_digest(void) {
    unsigned i;
    unsigned digest = 19u;
    for (i = 0u; i < OWN14_LANES; ++i) {
        digest = (digest * 149u) ^ (g_generation[i] * 11u + g_release_count[i] * 7u + (unsigned)g_active[i]);
    }
    return digest;
}
