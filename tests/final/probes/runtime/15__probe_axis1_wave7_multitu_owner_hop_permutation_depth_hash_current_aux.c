extern const unsigned* axis1_wave7_current_hops[1];
extern unsigned axis1_wave7_current_mix(unsigned value, unsigned weight);

unsigned axis1_wave7_owner_hop_depth_hash_current(unsigned seed) {
    const unsigned* hop = axis1_wave7_current_hops[0];
    unsigned acc = axis1_wave7_current_mix(seed + hop[0], hop[1]);
    return axis1_wave7_current_mix(acc, hop[0]);
}
