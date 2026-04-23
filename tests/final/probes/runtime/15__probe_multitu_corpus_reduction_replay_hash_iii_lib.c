static unsigned g3_state[8];
static unsigned g3_fold;

static unsigned rotl32(unsigned x, unsigned s) {
    unsigned r = s & 31u;
    if (r == 0u) {
        return x;
    }
    return (x << r) | (x >> (32u - r));
}

void crh3_reset(unsigned seed) {
    unsigned i;
    g3_fold = 2166136261u ^ (seed * 157u + 19u);
    for (i = 0u; i < 8u; ++i) {
        g3_state[i] = seed * (i + 31u) + i * 109u + 0xBEEF1u;
    }
}

unsigned crh3_step(unsigned lane, unsigned opcode, unsigned arg) {
    unsigned idx = lane & 7u;
    unsigned x = g3_state[idx];
    unsigned out;

    switch (opcode & 7u) {
        case 0u: out = (x + arg + idx * 5u) ^ 0x11u; break;
        case 1u: out = (x ^ (arg * 3u + idx)) + 0x77u; break;
        case 2u: out = x * 127u + (arg | 1u); break;
        case 3u: out = rotl32(x + arg, idx + 1u) ^ (arg * 13u); break;
        case 4u: out = (x - (arg & 0x1FFu)) ^ 0xA55Au; break;
        case 5u: out = (x + arg * 41u) ^ (x >> 3); break;
        case 6u: out = rotl32(x ^ (arg + idx * 17u), idx + 4u) + 0x9E37u; break;
        default: out = (x ^ (arg * 11u + 23u)) + (idx * 29u); break;
    }

    g3_state[idx] = out;
    g3_fold ^= out + idx * 47u;
    g3_fold *= 16777619u;
    return out ^ g3_fold;
}

unsigned crh3_digest(void) {
    return g3_fold ^ g3_state[1] ^ g3_state[3] ^ g3_state[5] ^ g3_state[7];
}
