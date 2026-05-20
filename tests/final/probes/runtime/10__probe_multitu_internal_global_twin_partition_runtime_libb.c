static int bucket10_private_state = 20;

int bucket10_twin_step_b(int x) {
    bucket10_private_state = bucket10_private_state - x + 5;
    return bucket10_private_state;
}

int bucket10_twin_peek_b(void) {
    return bucket10_private_state;
}
