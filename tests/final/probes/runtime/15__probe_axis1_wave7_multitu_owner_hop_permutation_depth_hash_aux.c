extern const unsigned axis1_wave7_hop_weights[10];
extern const unsigned* axis1_wave7_owner_pairs[4][2];
extern const unsigned* const* axis1_wave7_owner_pair_ptrs[4];
extern unsigned (*axis1_wave7_hop_mix)(unsigned, unsigned);

unsigned axis1_wave7_owner_hop_depth_hash(unsigned seed) {
    unsigned acc = seed;
    unsigned lane = 0;

    (void)axis1_wave7_owner_pairs;

    for (; lane < 4u; ++lane) {
        const unsigned* const* pair = axis1_wave7_owner_pair_ptrs[lane];
        const unsigned* left = pair[(lane + 1u) & 1u];
        const unsigned* right = pair[lane & 1u];
        acc = axis1_wave7_hop_mix(acc + left[0], right[1]);
        acc = axis1_wave7_hop_mix(acc + axis1_wave7_hop_weights[lane], left[1]);
    }

    return acc ^ axis1_wave7_hop_weights[9];
}
