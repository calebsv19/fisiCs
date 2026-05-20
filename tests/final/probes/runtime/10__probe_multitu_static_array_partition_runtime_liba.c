static int lane[3] = {3, 5, 7};

int bucket10_local_array_step_a(int index, int delta) {
    lane[index] += delta;
    return lane[0] + lane[1] + lane[2];
}

int bucket10_local_array_peek_a(void) {
    return lane[0] + lane[1] + lane[2];
}
