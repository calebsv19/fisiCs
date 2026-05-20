static int bucket10_private_lane[3] = {5, 1, 3};

int bucket10_internal_array_step_b(int index, int delta) {
    bucket10_private_lane[index] += delta;
    return bucket10_private_lane[0] + bucket10_private_lane[1] + bucket10_private_lane[2];
}

int bucket10_internal_array_peek_b(void) {
    return bucket10_private_lane[0] + bucket10_private_lane[1] + bucket10_private_lane[2];
}
