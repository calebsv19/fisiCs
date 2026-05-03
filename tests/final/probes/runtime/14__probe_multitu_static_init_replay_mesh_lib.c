static unsigned g_boot_bias = 41u;
static unsigned g_state;
static unsigned g_generation;
static unsigned g_rounds;

static unsigned boot_token(void) {
    static int booted;
    static unsigned token;

    if (!booted) {
        token = g_boot_bias * 19u + 7u;
        g_generation = token % 1021u;
        booted = 1;
    }
    return token;
}

unsigned sir_seed(void) {
    return boot_token();
}

void sir_reset(unsigned salt) {
    unsigned token = boot_token();
    g_state = token ^ (salt * 257u + 13u);
    g_generation = (token + salt * 97u + 23u) % 65521u;
    g_rounds = 0u;
}

unsigned sir_step(unsigned lane, unsigned weight) {
    if (g_state == 0u) {
        sir_reset(1u);
    }

    g_state = (g_state * 109u + lane * (weight + 5u) + g_generation + 0x9E37u) % 1000003u;
    g_generation = (g_generation + lane * 17u + weight * 13u + 19u) % 65521u;
    ++g_rounds;
    return g_state ^ (g_generation + g_rounds * 23u);
}

unsigned sir_snapshot(void) {
    return boot_token() ^ g_state ^ (g_generation << 1) ^ (g_rounds * 97u);
}
