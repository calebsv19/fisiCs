static unsigned bucket10_link_b_state[2];

unsigned bucket10_link_b_reset(unsigned seed) {
    bucket10_link_b_state[0] = seed + 5u;
    bucket10_link_b_state[1] = seed * 3u + 1u;
    return bucket10_link_b_state[0] ^ bucket10_link_b_state[1];
}

unsigned bucket10_link_b_mix(unsigned acc, unsigned lane, unsigned value) {
    unsigned idx = lane & 1u;
    bucket10_link_b_state[idx] = bucket10_link_b_state[idx] + value * 11u + lane * 13u;
    return (acc ^ bucket10_link_b_state[idx]) + bucket10_link_b_state[idx ^ 1u];
}

unsigned bucket10_link_b_peek(void) {
    return bucket10_link_b_state[0] + bucket10_link_b_state[1] * 5u;
}
