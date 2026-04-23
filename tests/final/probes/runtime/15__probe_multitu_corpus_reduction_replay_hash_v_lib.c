static unsigned g5_state[8];
static unsigned g5_fold;

static unsigned rotl32(unsigned x, unsigned s) {
    unsigned r = s & 31u;
    if (r == 0u) {
        return x;
    }
    return (x << r) | (x >> (32u - r));
}

void crh5_reset(unsigned seed) {
    unsigned i;
    g5_fold = 2166136261u ^ (seed * 191u + 29u);
    for (i = 0u; i < 8u; ++i) {
        g5_state[i] = seed * (i + 41u) + i * 127u + 0xD00D1u;
    }
}

unsigned crh5_step(unsigned lane, unsigned opcode, unsigned arg) {
    unsigned idx = lane & 7u;
    unsigned x = g5_state[idx];
    unsigned out;

    switch (opcode & 7u) {
        case 0u: out = (x + arg + idx * 9u) ^ 0x31u; break;
        case 1u: out = (x ^ (arg * 7u + idx)) + 0xABu; break;
        case 2u: out = x * 149u + (arg | 1u); break;
        case 3u: out = rotl32(x + arg, idx + 3u) ^ (arg * 23u); break;
        case 4u: out = (x - (arg & 0x7FFu)) ^ 0xC55Cu; break;
        case 5u: out = (x + arg * 47u) ^ (x >> 5); break;
        case 6u: out = rotl32(x ^ (arg + idx * 27u), idx + 6u) + 0xBE37u; break;
        default: out = (x ^ (arg * 17u + 31u)) + (idx * 37u); break;
    }

    g5_state[idx] = out;
    g5_fold ^= out + idx * 59u;
    g5_fold *= 16777619u;
    return out ^ g5_fold;
}

unsigned crh5_digest(void) {
    return g5_fold ^ g5_state[1] ^ g5_state[2] ^ g5_state[4] ^ g5_state[6];
}
