static int bucket10_private_state = 3;

int bucket10_twin_step_a(int x) {
    bucket10_private_state = bucket10_private_state * 2 + x;
    return bucket10_private_state;
}

int bucket10_twin_peek_a(void) {
    return bucket10_private_state;
}
