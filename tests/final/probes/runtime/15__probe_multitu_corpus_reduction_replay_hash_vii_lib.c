static unsigned g7_state[8];
static unsigned g7_fold;

static unsigned rotr32(unsigned x, unsigned s) {
    unsigned r = s & 31u;
    if (r == 0u) {
        return x;
    }
    return (x >> r) | (x << (32u - r));
}

void crh7_reset(unsigned seed) {
    unsigned i;
    g7_fold = 2166136261u ^ (seed * 251u + 37u);
    for (i = 0u; i < 8u; ++i) {
        g7_state[i] = seed * (i + 61u) + i * 173u + 0xC0FFEu;
    }
}

unsigned crh7_step(unsigned lane, unsigned opcode, unsigned arg) {
    unsigned idx = lane & 7u;
    unsigned x = g7_state[idx];
    unsigned out;

    switch (opcode & 7u) {
        case 0u: out = rotr32(x + arg + idx * 5u, idx + 1u) ^ 0x51u; break;
        case 1u: out = (x ^ (arg * 5u + idx * 7u)) + 0x1ABu; break;
        case 2u: out = x * 181u + (arg | 5u); break;
        case 3u: out = rotr32(x ^ arg, idx + 3u) + (arg * 17u); break;
        case 4u: out = (x - (arg & 0x9FFu)) ^ 0x5AA5u; break;
        case 5u: out = (x + arg * 31u) ^ (x >> 3); break;
        case 6u: out = rotr32(x + (arg ^ idx * 19u), idx + 6u) + 0x9E37u; break;
        default: out = (x ^ (arg * 23u + 17u)) + (idx * 29u); break;
    }

    g7_state[idx] = out;
    g7_fold ^= out + idx * 71u;
    g7_fold *= 16777619u;
    return out ^ g7_fold;
}

unsigned crh7_digest(void) {
    return g7_fold ^ g7_state[1] ^ g7_state[2] ^ g7_state[4] ^ g7_state[6];
}
