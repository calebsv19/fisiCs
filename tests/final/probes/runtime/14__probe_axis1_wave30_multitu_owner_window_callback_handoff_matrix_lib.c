typedef unsigned (*axis1_wave30_owner_fn)(unsigned, const unsigned*, unsigned);

static unsigned axis1_wave30_rotl(unsigned value, unsigned shift) {
    shift &= 31u;
    return shift ? ((value << shift) | (value >> (32u - shift))) : value;
}

static unsigned axis1_wave30_owner_step_fold(unsigned value, const unsigned* window, unsigned salt) {
    return axis1_wave30_rotl(value + window[0] + salt + 0x21u, (window[1] & 7u) + 1u);
}

static unsigned axis1_wave30_owner_step_shadow(unsigned value, const unsigned* window, unsigned salt) {
    return (value ^ (window[1] + 0x63u)) + axis1_wave30_rotl(window[0] + salt, 3u);
}

static unsigned axis1_wave30_owner_step_handoff(unsigned value, const unsigned* window, unsigned salt) {
    return (value * 17u) ^ axis1_wave30_rotl(window[0] + window[1] + salt, 5u);
}

axis1_wave30_owner_fn axis1_wave30_owner_table[3] = {
    axis1_wave30_owner_step_fold,
    axis1_wave30_owner_step_shadow,
    axis1_wave30_owner_step_handoff,
};

const unsigned axis1_wave30_owner_weights[18] = {
    3u, 7u, 13u, 19u, 29u, 31u, 43u, 47u, 59u,
    61u, 73u, 79u, 89u, 97u, 101u, 109u, 127u, 131u,
};
const unsigned* axis1_wave30_owner_windows[7] = {
    axis1_wave30_owner_weights + 0,
    axis1_wave30_owner_weights + 2,
    axis1_wave30_owner_weights + 4,
    axis1_wave30_owner_weights + 6,
    axis1_wave30_owner_weights + 8,
    axis1_wave30_owner_weights + 10,
    axis1_wave30_owner_weights + 12,
};
const unsigned** axis1_wave30_owner_routes[6] = {
    axis1_wave30_owner_windows + 1,
    axis1_wave30_owner_windows + 3,
    axis1_wave30_owner_windows + 5,
    axis1_wave30_owner_windows + 0,
    axis1_wave30_owner_windows + 2,
    axis1_wave30_owner_windows + 4,
};
const unsigned axis1_wave30_handoff_perm[8] = {1u, 6u, 2u, 7u, 0u, 5u, 3u, 4u};

unsigned axis1_wave30_route_state = 2u;
unsigned axis1_wave30_epoch_state = 5u;

axis1_wave30_owner_fn axis1_wave30_owner_pick(unsigned lane, unsigned checkpoint) {
    unsigned pick =
        (axis1_wave30_handoff_perm[(lane + checkpoint + axis1_wave30_route_state) % 8u] +
         axis1_wave30_epoch_state) %
        3u;
    return axis1_wave30_owner_table[pick];
}
