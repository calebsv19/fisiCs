extern const unsigned* axis1_wave6_owner_hop_reduced_hops[1];
extern unsigned axis1_wave6_owner_hop_reduced_mix(unsigned value, unsigned weight);

unsigned axis1_wave6_owner_hop_reduced_fold(unsigned seed) {
    const unsigned* hop = axis1_wave6_owner_hop_reduced_hops[0];
    unsigned acc = axis1_wave6_owner_hop_reduced_mix(seed, hop[0]);
    return axis1_wave6_owner_hop_reduced_mix(acc, hop[1]);
}
