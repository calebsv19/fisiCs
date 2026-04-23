static unsigned g_state[8];
static unsigned g_crc;

static unsigned rotl_u32(unsigned x, unsigned s) {
    unsigned r = s & 31u;
    if (r == 0u) {
        return x;
    }
    return (x << r) | (x >> (32u - r));
}

void crh2_reset(unsigned seed) {
    unsigned i;
    g_crc = 2166136261u ^ (seed * 131u + 7u);
    for (i = 0u; i < 8u; ++i) {
        g_state[i] = seed * (i + 23u) + i * 97u + 0x9E37u;
    }
}

unsigned crh2_step(unsigned lane, unsigned opcode, unsigned arg) {
    unsigned idx = lane & 7u;
    unsigned x = g_state[idx];
    unsigned out;

    switch (opcode & 7u) {
        case 0u: out = x + arg + idx * 3u; break;
        case 1u: out = (x ^ arg) + 0x55u + idx; break;
        case 2u: out = x * 131u + (arg | 1u); break;
        case 3u: out = (x + (arg >> 1)) ^ (arg * 17u); break;
        case 4u: out = rotl_u32(x ^ (arg + idx * 11u), (idx + 3u)); break;
        case 5u: out = (x - (arg & 0xFFu)) ^ 0xA5A5u; break;
        case 6u: out = (x + arg * 29u) ^ (x >> 2); break;
        default: out = (x ^ (arg * 7u + 13u)) + (idx * 19u); break;
    }

    g_state[idx] = out;
    g_crc ^= out + idx * 41u;
    g_crc *= 16777619u;
    return out ^ g_crc;
}

unsigned crh2_digest(void) {
    return g_crc ^ g_state[0] ^ g_state[2] ^ g_state[4] ^ g_state[6];
}
