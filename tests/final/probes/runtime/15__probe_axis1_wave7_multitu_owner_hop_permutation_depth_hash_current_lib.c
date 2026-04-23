const unsigned axis1_wave7_current_weights[2] = {9u, 21u};
const unsigned* axis1_wave7_current_hops[1] = {
    axis1_wave7_current_weights,
};

unsigned axis1_wave7_current_mix(unsigned value, unsigned weight) {
    return (value * 11u) + (weight ^ 0x31u);
}
