typedef unsigned (*axis1_wave10_checkpoint_fn)(unsigned, unsigned);

static unsigned axis1_wave10_checkpoint_add(unsigned value, unsigned weight) {
    return (value + weight) * 9u;
}

static unsigned axis1_wave10_checkpoint_xor(unsigned value, unsigned weight) {
    return (value ^ (weight + 0x4Du)) + (value >> 1);
}

static unsigned axis1_wave10_checkpoint_mix_bias(unsigned value, unsigned weight) {
    return (value * 15u) + (weight ^ 0x57u);
}

static unsigned axis1_wave10_checkpoint_fold(unsigned value, unsigned weight) {
    return (value + (weight * 11u)) ^ 0xE1u;
}

axis1_wave10_checkpoint_fn axis1_wave10_checkpoint_table[4] = {
    axis1_wave10_checkpoint_add,
    axis1_wave10_checkpoint_xor,
    axis1_wave10_checkpoint_mix_bias,
    axis1_wave10_checkpoint_fold,
};

const unsigned axis1_wave10_checkpoint_order[6] = {1u, 3u, 0u, 2u, 1u, 0u};
const unsigned axis1_wave10_checkpoint_bias[6] = {3u, 8u, 13u, 18u, 23u, 28u};
