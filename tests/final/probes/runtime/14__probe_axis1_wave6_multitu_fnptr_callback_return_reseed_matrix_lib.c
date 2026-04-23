static int axis1_wave6_step_add7(int value) {
    return value + 7;
}

static int axis1_wave6_step_xor13(int value) {
    return value ^ 13;
}

static int axis1_wave6_step_mul3_add1(int value) {
    return (value * 3) + 1;
}

int (*axis1_wave6_callback_pick(int lane))(int) {
    if (lane == 0) {
        return axis1_wave6_step_add7;
    }
    if (lane == 1) {
        return axis1_wave6_step_xor13;
    }
    return axis1_wave6_step_mul3_add1;
}
