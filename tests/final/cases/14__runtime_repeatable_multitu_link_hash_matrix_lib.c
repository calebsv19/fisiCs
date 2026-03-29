static unsigned fnv1a_u32(unsigned h, unsigned v) {
    h ^= (v & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 8) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 16) & 0xFFu);
    h *= 16777619u;
    h ^= ((v >> 24) & 0xFFu);
    h *= 16777619u;
    return h;
}

static unsigned lane_step(unsigned lane, unsigned x) {
    switch (lane & 3u) {
        case 0u: return (x + lane * 7u) ^ 0xA5A5u;
        case 1u: return (x * (lane + 3u)) + 0x1021u;
        case 2u: return (x ^ (lane * 131u + 17u)) + (x >> 1);
        default: return (x + (lane + 11u) * (lane + 5u)) ^ (x << 3);
    }
}

unsigned multitu_lane_hash(unsigned lane, unsigned seed) {
    unsigned h = 2166136261u;
    unsigned x = seed + lane * 19u + 29u;
    int i;
    for (i = 0; i < 6; ++i) {
        x = lane_step(lane + (unsigned)i, x + (unsigned)i * 13u);
        h = fnv1a_u32(h, x);
    }
    return h;
}

unsigned multitu_lane_mix(unsigned acc, unsigned lane_hash, unsigned step) {
    unsigned mixed = acc ^ (lane_hash + step * 97u);
    return fnv1a_u32(mixed, lane_hash ^ (step * 131u + 7u));
}
