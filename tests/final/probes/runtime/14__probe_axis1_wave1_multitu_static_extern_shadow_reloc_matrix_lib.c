static int axis1_w1_shadow_token = 31;

int axis1_w1_shadow_read_local(int step) {
    axis1_w1_shadow_token += (step % 3) + 1;
    return axis1_w1_shadow_token * 5 + step;
}

int axis1_w1_shadow_mix_external(int ext_value, int step) {
    int mix = ext_value * 13 + axis1_w1_shadow_token * 7 + step;
    axis1_w1_shadow_token ^= (step * 17 + 3);
    return mix ^ axis1_w1_shadow_token;
}
