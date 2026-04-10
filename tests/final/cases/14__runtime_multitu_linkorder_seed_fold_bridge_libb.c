static unsigned wave93_b_state[4];

static unsigned wave93_b_boot(void) {
    static unsigned booted;
    static unsigned token;
    if (!booted) {
        token = 2166136261u ^ 0x93u;
        booted = 1u;
    }
    return token;
}

unsigned wave93_b_reset(unsigned seed) {
    unsigned i;
    unsigned boot = wave93_b_boot();
    for (i = 0u; i < 4u; ++i) {
        wave93_b_state[i] = seed * (i + 5u) + boot + i * 4099u + (i + 1u) * 17u;
    }
    return wave93_b_state[0] ^ wave93_b_state[3];
}

unsigned wave93_b_mix(unsigned acc, unsigned lane, unsigned value) {
    unsigned idx = lane & 3u;
    unsigned roll = wave93_b_state[(idx + 1u) & 3u];
    wave93_b_state[idx] = (wave93_b_state[idx] + value * 167u + (lane + 1u) * 97u) ^ (roll >> 1);
    return (acc ^ wave93_b_state[idx]) * 16777619u + (lane * 131u + 17u);
}

unsigned wave93_b_snapshot(void) {
    return wave93_b_state[0] + wave93_b_state[1] * 3u + wave93_b_state[2] * 5u + wave93_b_state[3] * 7u;
}
