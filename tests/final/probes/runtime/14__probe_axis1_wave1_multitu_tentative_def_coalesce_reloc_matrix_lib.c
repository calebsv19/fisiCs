extern int axis1_w1_seed;

int axis1_w1_table[6] = {3, 11, 7, 19, 5, 23};

int axis1_w1_mix_lane(int lane, int salt) {
    int idx = (lane + salt + axis1_w1_seed) % 6;
    int base = axis1_w1_table[idx] + axis1_w1_seed * 3 + lane;
    return base ^ (salt * 5 + idx);
}

int axis1_w1_step_counter(int delta) {
    axis1_w1_seed += delta;
    if (axis1_w1_seed < 0) {
        axis1_w1_seed = -axis1_w1_seed + 1;
    }
    return axis1_w1_seed * 7 + axis1_w1_table[(axis1_w1_seed + 1) % 6];
}
