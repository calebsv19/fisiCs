typedef unsigned (*axis1_wave29_step_fn)(unsigned, unsigned, unsigned);

static unsigned axis1_wave29_rotl(unsigned value, unsigned shift) {
    shift &= 31u;
    return shift ? ((value << shift) | (value >> (32u - shift))) : value;
}

static unsigned axis1_wave29_step_add(unsigned value, unsigned weight, unsigned salt) {
    return axis1_wave29_rotl(value + weight + salt + 0x31u, (salt & 7u) + 1u);
}

static unsigned axis1_wave29_step_xor(unsigned value, unsigned weight, unsigned salt) {
    return (value ^ (weight + 0x53u)) + axis1_wave29_rotl(salt + value, 3u);
}

static unsigned axis1_wave29_step_mix(unsigned value, unsigned weight, unsigned salt) {
    return (value * 33u) ^ axis1_wave29_rotl(weight + salt + 0x79u, 5u);
}

static unsigned axis1_wave29_step_fold(unsigned value, unsigned weight, unsigned salt) {
    return axis1_wave29_rotl(value + (weight * 7u) + (salt ^ 0xA5u), 9u) ^ 0xC3u;
}

axis1_wave29_step_fn axis1_wave29_step_table[4] = {
    axis1_wave29_step_add,
    axis1_wave29_step_xor,
    axis1_wave29_step_mix,
    axis1_wave29_step_fold,
};

const unsigned axis1_wave29_perm[9] = {3u, 1u, 6u, 0u, 8u, 2u, 5u, 4u, 7u};
const unsigned axis1_wave29_bias[9] = {5u, 11u, 17u, 23u, 29u, 37u, 43u, 47u, 53u};
const unsigned axis1_wave29_route_weights[16] = {
    7u, 19u, 31u, 43u, 59u, 71u, 83u, 97u,
    109u, 127u, 149u, 163u, 181u, 193u, 211u, 233u,
};
const unsigned axis1_wave29_shadow_salts[9] = {2u, 4u, 7u, 10u, 14u, 19u, 23u, 28u, 34u};

unsigned axis1_wave29_epoch_state = 3u;
unsigned axis1_wave29_owner_state = 1u;

axis1_wave29_step_fn axis1_wave29_callback_pick(unsigned lane, unsigned replay) {
    unsigned pick =
        (axis1_wave29_perm[(lane + axis1_wave29_epoch_state + replay) % 9u] +
         axis1_wave29_owner_state) &
        3u;
    return axis1_wave29_step_table[pick];
}
