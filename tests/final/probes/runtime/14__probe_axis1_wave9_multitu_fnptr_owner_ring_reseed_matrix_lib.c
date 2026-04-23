typedef unsigned (*axis1_wave9_ring_fn)(unsigned, unsigned);

static unsigned axis1_wave9_ring_add(unsigned value, unsigned weight) {
    return (value + weight) * 5u;
}

static unsigned axis1_wave9_ring_xor(unsigned value, unsigned weight) {
    return (value ^ (weight + 0x2Bu)) + (value >> 2);
}

static unsigned axis1_wave9_ring_mix_bias(unsigned value, unsigned weight) {
    return (value * 13u) + (weight ^ 0x91u);
}

static unsigned axis1_wave9_ring_fold(unsigned value, unsigned weight) {
    return (value + (weight * 7u)) ^ 0xC3u;
}

axis1_wave9_ring_fn axis1_wave9_ring_table[4] = {
    axis1_wave9_ring_add,
    axis1_wave9_ring_xor,
    axis1_wave9_ring_mix_bias,
    axis1_wave9_ring_fold,
};

const unsigned axis1_wave9_ring_perm[5] = {2u, 0u, 3u, 1u, 2u};
const unsigned axis1_wave9_ring_bias[5] = {4u, 9u, 15u, 21u, 27u};
