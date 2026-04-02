unsigned mt15_mix(unsigned x, unsigned y) {
    unsigned v = x ^ (y + 0x9e3779b9u + (x << 6) + (x >> 2));
    v ^= v >> 15u;
    v *= 0x85ebca6bu;
    v ^= v >> 13u;
    v *= 0xc2b2ae35u;
    v ^= v >> 16u;
    return v;
}
