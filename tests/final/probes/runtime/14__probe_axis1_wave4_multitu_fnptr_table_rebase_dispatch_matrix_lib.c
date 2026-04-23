static int axis1_wave4_step_add3(int value) {
    return value + 3;
}

static int axis1_wave4_step_xor5(int value) {
    return value ^ 5;
}

int (*axis1_wave4_dispatch_table[2])(int) = {
    axis1_wave4_step_add3,
    axis1_wave4_step_xor5,
};
