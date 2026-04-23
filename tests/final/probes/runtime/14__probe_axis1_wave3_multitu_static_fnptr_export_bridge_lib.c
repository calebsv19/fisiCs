static int axis1_wave3_step_add(int value) {
    return value + 4;
}

static int axis1_wave3_step_xor(int value) {
    return value ^ 9;
}

int (*axis1_wave3_pick(int lane))(int) {
    if (lane) {
        return axis1_wave3_step_xor;
    }
    return axis1_wave3_step_add;
}
