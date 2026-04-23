#define LB14_LANES 3u

static unsigned g_generation[LB14_LANES];
static unsigned g_resident[LB14_LANES];
static unsigned g_trimmed[LB14_LANES];
static int g_active[LB14_LANES];

void lb14_reset(void) {
    unsigned i;
    for (i = 0u; i < LB14_LANES; ++i) {
        g_generation[i] = 0u;
        g_resident[i] = 0u;
        g_trimmed[i] = 0u;
        g_active[i] = 0;
    }
}

void lb14_toggle(unsigned lane, int enabled) {
    unsigned idx = lane % LB14_LANES;
    if (enabled) {
        if (!g_active[idx]) {
            g_active[idx] = 1;
            g_generation[idx] += 1u;
            g_resident[idx] += 1u + idx;
        }
        return;
    }

    if (g_active[idx]) {
        g_active[idx] = 0;
        if (g_resident[idx] > 0u) {
            g_resident[idx] -= 1u;
        }
        g_trimmed[idx] += 1u;
    }
}

void lb14_touch(unsigned lane, unsigned tiles) {
    unsigned idx = lane % LB14_LANES;
    if (g_active[idx]) {
        g_resident[idx] += (tiles % 5u) + 1u;
    } else if (g_resident[idx] > 0u) {
        g_resident[idx] -= 1u;
    }
}

unsigned lb14_token(unsigned lane) {
    return g_generation[lane % LB14_LANES];
}

int lb14_collect(unsigned lane, unsigned generation, unsigned* out_value) {
    unsigned idx = lane % LB14_LANES;
    if (g_generation[idx] != generation || g_resident[idx] == 0u) {
        return 0;
    }
    if (out_value) {
        *out_value = g_resident[idx] * 23u + g_trimmed[idx] * 7u + generation;
    }
    return 1;
}

unsigned lb14_digest(void) {
    unsigned i;
    unsigned digest = 11u;
    for (i = 0u; i < LB14_LANES; ++i) {
        digest = (digest * 181u)
            ^ (g_generation[i] * 13u)
            ^ (g_resident[i] * 17u)
            ^ (g_trimmed[i] * 19u)
            ^ ((unsigned)g_active[i] * 29u);
    }
    return digest;
}
