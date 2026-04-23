extern int (*axis1_wave6_callback_pick(int lane))(int);

static int axis1_wave6_apply_lane(int seed, int lane, int salt) {
    int (*step)(int) = axis1_wave6_callback_pick((lane + salt) % 3);
    return step(seed + salt);
}

int axis1_wave6_callback_eval(int seed) {
    int stage0 = axis1_wave6_apply_lane(seed, 0, 2);
    int stage1 = axis1_wave6_apply_lane(stage0, 1, 1);
    int stage2 = axis1_wave6_apply_lane(stage1, 2, 0);
    return stage2 + axis1_wave6_apply_lane(1, seed % 3, 0);
}
