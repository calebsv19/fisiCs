const unsigned axis1_wave6_owner_hop_reduced_weights[2] = {5u, 9u};
const unsigned* axis1_wave6_owner_hop_reduced_hops[1] = {
    axis1_wave6_owner_hop_reduced_weights,
};

unsigned axis1_wave6_owner_hop_reduced_mix(unsigned value, unsigned weight) {
    return (value * 13u) + weight;
}
