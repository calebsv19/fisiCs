static int bucket10_link_a_state[3];

int bucket10_link_a_reset(int seed) {
    int i;
    for (i = 0; i < 3; ++i) {
        bucket10_link_a_state[i] = seed + (i + 1) * 7;
    }
    return bucket10_link_a_state[0] + bucket10_link_a_state[2];
}

int bucket10_link_a_step(int lane, int x) {
    int idx = lane % 3;
    bucket10_link_a_state[idx] = bucket10_link_a_state[idx] * 3 + x + idx;
    return bucket10_link_a_state[idx];
}

int bucket10_link_a_peek(void) {
    return bucket10_link_a_state[0] - bucket10_link_a_state[1] + bucket10_link_a_state[2];
}
