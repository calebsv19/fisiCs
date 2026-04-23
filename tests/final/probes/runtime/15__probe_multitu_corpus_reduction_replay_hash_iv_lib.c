static unsigned g4_state[8];
static unsigned g4_fold;

static unsigned rotl32(unsigned x, unsigned s) {
    unsigned r = s & 31u;
    if (r == 0u) {
        return x;
    }
    return (x << r) | (x >> (32u - r));
}

void crh4_reset(unsigned seed) {
    unsigned i;
    g4_fold = 2166136261u ^ (seed * 173u + 23u);
    for (i = 0u; i < 8u; ++i) {
        g4_state[i] = seed * (i + 37u) + i * 113u + 0xCAFE1u;
    }
}

unsigned crh4_step(unsigned lane, unsigned opcode, unsigned arg) {
    unsigned idx = lane & 7u;
    unsigned x = g4_state[idx];
    unsigned out;

    switch (opcode & 7u) {
        case 0u: out = (x + arg + idx * 7u) ^ 0x21u; break;
        case 1u: out = (x ^ (arg * 5u + idx)) + 0x99u; break;
        case 2u: out = x * 139u + (arg | 1u); break;
        case 3u: out = rotl32(x + arg, idx + 2u) ^ (arg * 19u); break;
        case 4u: out = (x - (arg & 0x3FFu)) ^ 0xB55Bu; break;
        case 5u: out = (x + arg * 43u) ^ (x >> 4); break;
        case 6u: out = rotl32(x ^ (arg + idx * 23u), idx + 5u) + 0xAE37u; break;
        default: out = (x ^ (arg * 13u + 29u)) + (idx * 31u); break;
    }

    g4_state[idx] = out;
    g4_fold ^= out + idx * 53u;
    g4_fold *= 16777619u;
    return out ^ g4_fold;
}

unsigned crh4_digest(void) {
    return g4_fold ^ g4_state[0] ^ g4_state[3] ^ g4_state[5] ^ g4_state[7];
}
