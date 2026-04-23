static unsigned axis1_wave7_hop_mix_impl(unsigned value, unsigned weight) {
    return ((value + (weight * 5u)) ^ 0x12Du) + (value >> 2);
}

const unsigned axis1_wave7_hop_weights[10] = {4u, 8u, 12u, 16u, 20u, 24u, 28u, 32u, 36u, 40u};
const unsigned* axis1_wave7_owner_pairs[4][2] = {
    {axis1_wave7_hop_weights + 0, axis1_wave7_hop_weights + 2},
    {axis1_wave7_hop_weights + 2, axis1_wave7_hop_weights + 4},
    {axis1_wave7_hop_weights + 4, axis1_wave7_hop_weights + 6},
    {axis1_wave7_hop_weights + 6, axis1_wave7_hop_weights + 8},
};
const unsigned* const* axis1_wave7_owner_pair_ptrs[4] = {
    axis1_wave7_owner_pairs[2],
    axis1_wave7_owner_pairs[0],
    axis1_wave7_owner_pairs[3],
    axis1_wave7_owner_pairs[1],
};
unsigned (*axis1_wave7_hop_mix)(unsigned, unsigned) = axis1_wave7_hop_mix_impl;
