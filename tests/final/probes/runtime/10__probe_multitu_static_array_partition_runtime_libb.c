static int lane[3] = {2, 4, 6};

int bucket10_local_array_step_b(int index, int delta) {
    lane[index] += delta;
    return lane[0] + lane[1] + lane[2];
}

int bucket10_local_array_peek_b(void) {
    return lane[0] + lane[1] + lane[2];
}
