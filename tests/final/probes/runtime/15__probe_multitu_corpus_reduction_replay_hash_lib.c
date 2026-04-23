static unsigned g_lane_state[4];
static unsigned g_fold;

static unsigned mix_step(unsigned x, unsigned y, unsigned salt) {
    unsigned v = x ^ (y + salt * 97u + 11u);
    v = (v << ((salt & 7u) + 1u)) | (v >> (32u - ((salt & 7u) + 1u)));
    return v + x * 13u + y * 5u;
}

void crh_reset(unsigned seed) {
    unsigned i;
    g_fold = 2166136261u ^ seed;
    for (i = 0u; i < 4u; ++i) {
        g_lane_state[i] = seed * (i + 17u) + 0x9E37u + i * 131u;
    }
}

unsigned crh_step(unsigned lane, unsigned opcode, unsigned arg) {
    unsigned idx = lane & 3u;
    unsigned x = g_lane_state[idx];
    unsigned out;

    if (opcode == 0u) {
        out = mix_step(x, arg, lane + 1u);
    } else if (opcode == 1u) {
        out = (x + (arg | 1u) * 17u) ^ (arg >> 2);
    } else if (opcode == 2u) {
        out = (x ^ (arg * 29u + 3u)) + (x >> 1);
    } else {
        out = (x * 131u) + (arg ^ (lane * 7u + 5u));
    }

    g_lane_state[idx] = out;
    g_fold ^= out + idx * 19u;
    g_fold *= 16777619u;
    return out ^ g_fold;
}

unsigned crh_digest(void) {
    return g_fold ^ g_lane_state[0] ^ g_lane_state[1] ^ g_lane_state[2] ^ g_lane_state[3];
}
