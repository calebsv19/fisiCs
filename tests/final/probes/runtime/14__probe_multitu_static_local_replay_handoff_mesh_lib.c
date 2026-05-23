static unsigned rotl32(unsigned value, unsigned shift) {
    shift &= 31u;
    return shift ? ((value << shift) | (value >> (32u - shift))) : value;
}

static unsigned g_seed_bias = 23u;
static unsigned g_epoch;
static unsigned g_handoff;

static unsigned boot_token(void) {
    static int booted;
    static unsigned token;

    if (!booted) {
        token = g_seed_bias * 37u + 11u;
        g_epoch = token % 1021u;
        booted = 1;
    }
    return token;
}

unsigned slrhm_seed(void) {
    return boot_token();
}

unsigned slrhm_step(unsigned lane, unsigned weight) {
    static unsigned state = 0u;
    static unsigned replay = 0u;
    unsigned token = boot_token();

    if (state == 0u) {
        state = token ^ 0x9E3779B9u;
    }

    state = rotl32(state + lane * 19u + weight * 13u + g_epoch + replay + 0x53u,
                   (lane + g_handoff) & 7u);
    if (((state >> ((lane & 3u) + 1u)) & 1u) != 0u) {
        g_handoff = (g_handoff + weight + (state & 7u)) % 23u;
    } else {
        replay = (replay + lane + (weight & 3u) + (state & 1u)) % 17u;
    }
    g_epoch = (g_epoch + lane * 17u + weight * 11u + replay + 5u) % 65521u;
    return state ^ (g_epoch * 29u) ^ (g_handoff * 31u) ^ replay;
}

unsigned slrhm_snapshot(void) {
    unsigned token = boot_token();
    return token ^ (g_epoch * 7u) ^ (g_handoff * 13u);
}
