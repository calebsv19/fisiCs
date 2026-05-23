typedef unsigned (*axis1_wave31_route_fn)(unsigned, unsigned, unsigned);

static unsigned axis1_wave31_rotl(unsigned value, unsigned shift) {
    shift &= 31u;
    return shift ? ((value << shift) | (value >> (32u - shift))) : value;
}

static unsigned axis1_wave31_route_add(unsigned value, unsigned weight, unsigned salt) {
    return axis1_wave31_rotl(value + weight + (salt ^ 0x35u), 3u);
}

static unsigned axis1_wave31_route_xor(unsigned value, unsigned weight, unsigned salt) {
    return (value ^ axis1_wave31_rotl(weight + 0x5Fu, salt & 7u)) + (salt * 9u);
}

static unsigned axis1_wave31_route_fold(unsigned value, unsigned weight, unsigned salt) {
    return (value * 19u) ^ axis1_wave31_rotl(weight + salt + 0x83u, 5u);
}

static unsigned axis1_wave31_route_handoff(unsigned value, unsigned weight, unsigned salt) {
    return axis1_wave31_rotl(value + (weight * 7u) + salt + 0x91u, 9u) ^ 0xD7u;
}

axis1_wave31_route_fn axis1_wave31_route_table[4] = {
    axis1_wave31_route_add,
    axis1_wave31_route_xor,
    axis1_wave31_route_fold,
    axis1_wave31_route_handoff,
};

const unsigned axis1_wave31_owner_perm[10] = {4u, 1u, 8u, 0u, 7u, 3u, 9u, 2u, 6u, 5u};
const unsigned axis1_wave31_owner_bias[10] = {6u, 12u, 18u, 25u, 33u, 39u, 46u, 54u, 63u, 71u};
const unsigned axis1_wave31_route_weights[20] = {
    5u, 11u, 17u, 23u, 31u, 41u, 53u, 67u, 79u, 97u,
    109u, 127u, 149u, 163u, 179u, 191u, 211u, 227u, 239u, 251u,
};
unsigned axis1_wave31_shadow_state = 4u;
unsigned axis1_wave31_checkpoint_state = 2u;

axis1_wave31_route_fn axis1_wave31_pick(unsigned lane, unsigned epoch) {
    unsigned pick =
        (axis1_wave31_owner_perm[(lane + epoch + axis1_wave31_shadow_state) % 10u] +
         axis1_wave31_checkpoint_state) &
        3u;
    return axis1_wave31_route_table[pick];
}
