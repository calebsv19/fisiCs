typedef unsigned (*PermFn)(unsigned x, unsigned y);

static unsigned lane_add(unsigned x, unsigned y) { return x + y + 3u; }
static unsigned lane_xor(unsigned x, unsigned y) { return (x ^ y) + 0x55u; }
static unsigned lane_mix(unsigned x, unsigned y) { return (x * 9u) + (y * 5u) + 7u; }
static unsigned lane_rot(unsigned x, unsigned y) { return (x << 3) ^ (x >> 2) ^ (y * 13u); }
static unsigned lane_sub(unsigned x, unsigned y) { return (x - y) ^ 0xA5A5u; }
static unsigned lane_or(unsigned x, unsigned y) { return (x | y) + (x & 0x33u) + 11u; }
static unsigned lane_mul(unsigned x, unsigned y) { return (x * (y | 1u)) + 19u; }
static unsigned lane_fold(unsigned x, unsigned y) { return ((x + y) * 131u) ^ (x >> 1); }

static PermFn k_symbol_table[8] = {
    lane_add, lane_xor, lane_mix, lane_rot,
    lane_sub, lane_or, lane_mul, lane_fold
};

PermFn perm_pick_symbol(unsigned idx) {
    return k_symbol_table[idx & 7u];
}

unsigned perm_symbol_count(void) {
    return 8u;
}
