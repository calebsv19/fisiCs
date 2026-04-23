const unsigned axis1_wave4_weights[4] = {3u, 5u, 7u, 11u};

unsigned axis1_wave4_mix(unsigned value) {
    return (value * 33u) ^ 0x9E3779B9u;
}
