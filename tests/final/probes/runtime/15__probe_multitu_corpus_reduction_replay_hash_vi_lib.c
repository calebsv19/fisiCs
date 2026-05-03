static unsigned g6_state[8];
static unsigned g6_fold;

static unsigned rotl32(unsigned x, unsigned s) {
    unsigned r = s & 31u;
    if (r == 0u) {
        return x;
    }
    return (x << r) | (x >> (32u - r));
}

void crh6_reset(unsigned seed) {
    unsigned i;
    g6_fold = 2166136261u ^ (seed * 223u + 31u);
    for (i = 0u; i < 8u; ++i) {
        g6_state[i] = seed * (i + 53u) + i * 149u + 0xE10F1u;
    }
}

unsigned crh6_step(unsigned lane, unsigned opcode, unsigned arg) {
    unsigned idx = lane & 7u;
    unsigned x = g6_state[idx];
    unsigned out;

    switch (opcode & 7u) {
        case 0u: out = rotl32(x + arg + idx * 7u, idx + 1u) ^ 0x13u; break;
        case 1u: out = (x ^ (arg * 11u + idx * 3u)) + 0xC7u; break;
        case 2u: out = x * 167u + (arg | 3u); break;
        case 3u: out = rotl32(x ^ arg, idx + 4u) + (arg * 19u); break;
        case 4u: out = (x - (arg & 0xFFFu)) ^ 0xA55Au; break;
        case 5u: out = (x + arg * 29u) ^ (x >> 7); break;
        case 6u: out = rotl32(x + (arg ^ idx * 23u), idx + 6u) + 0xBEADu; break;
        default: out = (x ^ (arg * 13u + 37u)) + (idx * 41u); break;
    }

    g6_state[idx] = out;
    g6_fold ^= out + idx * 67u;
    g6_fold *= 16777619u;
    return out ^ g6_fold;
}

unsigned crh6_digest(void) {
    return g6_fold ^ g6_state[0] ^ g6_state[3] ^ g6_state[5] ^ g6_state[7];
}
