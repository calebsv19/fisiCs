typedef unsigned (*axis1_wave7_mix_fn)(unsigned, unsigned);

static unsigned axis1_wave7_mix_addmul(unsigned value, unsigned weight) {
    return (value + weight) * 3u;
}

static unsigned axis1_wave7_mix_xorrot(unsigned value, unsigned weight) {
    return (value ^ (weight + 0x35u)) + (value >> 1);
}

static unsigned axis1_wave7_mix_bias(unsigned value, unsigned weight) {
    return (value * 17u) + (weight ^ 0x5Au);
}

static unsigned axis1_wave7_mix_fold(unsigned value, unsigned weight) {
    return (value + (weight * 9u)) ^ 0xA5u;
}

axis1_wave7_mix_fn axis1_wave7_mix_table[4] = {
    axis1_wave7_mix_addmul,
    axis1_wave7_mix_xorrot,
    axis1_wave7_mix_bias,
    axis1_wave7_mix_fold,
};

const unsigned axis1_wave7_owner_perm[4] = {2u, 0u, 3u, 1u};
const unsigned axis1_wave7_owner_bias[4] = {5u, 9u, 13u, 17u};
