static int axis1_wave3_reduced_step_add(int value) {
    return value + 4;
}

int (*axis1_wave3_reduced_pick)(int) = axis1_wave3_reduced_step_add;
