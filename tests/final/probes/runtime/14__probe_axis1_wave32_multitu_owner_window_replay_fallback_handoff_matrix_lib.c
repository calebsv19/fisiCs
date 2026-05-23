static unsigned axis1_wave32_owner_mix_impl(unsigned value, unsigned weight, unsigned salt) {
    unsigned shift = (salt & 7u) + 1u;
    return ((value ^ (weight + 0x6Bu)) * 23u) + ((value << shift) | (value >> (32u - shift)));
}

const unsigned axis1_wave32_owner_weights[22] = {
    9u, 15u, 21u, 27u, 35u, 41u, 49u, 57u, 69u, 75u, 83u,
    91u, 105u, 117u, 129u, 141u, 153u, 165u, 177u, 189u, 201u, 213u,
};
const unsigned* axis1_wave32_owner_windows[8] = {
    axis1_wave32_owner_weights + 0,
    axis1_wave32_owner_weights + 2,
    axis1_wave32_owner_weights + 4,
    axis1_wave32_owner_weights + 6,
    axis1_wave32_owner_weights + 8,
    axis1_wave32_owner_weights + 10,
    axis1_wave32_owner_weights + 12,
    axis1_wave32_owner_weights + 14,
};
const unsigned** axis1_wave32_owner_routes[7] = {
    axis1_wave32_owner_windows + 1,
    axis1_wave32_owner_windows + 3,
    axis1_wave32_owner_windows + 5,
    axis1_wave32_owner_windows + 0,
    axis1_wave32_owner_windows + 2,
    axis1_wave32_owner_windows + 4,
    axis1_wave32_owner_windows + 6,
};
const unsigned axis1_wave32_fallback_perm[9] = {2u, 8u, 1u, 7u, 0u, 6u, 3u, 5u, 4u};

unsigned axis1_wave32_route_state = 3u;
unsigned axis1_wave32_replay_state = 1u;

unsigned axis1_wave32_owner_mix(unsigned value, unsigned weight, unsigned salt) {
    return axis1_wave32_owner_mix_impl(value, weight, salt);
}
