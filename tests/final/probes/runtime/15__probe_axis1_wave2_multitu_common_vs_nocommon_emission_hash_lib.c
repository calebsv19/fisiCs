extern int axis1_w2_common_counter;
extern int axis1_w2_common_only;
extern int axis1_w2_common_aux_mix(int lane, int salt);

int axis1_w2_common_counter = 5;

int axis1_w2_common_tick(int lane, int salt) {
    int mix = axis1_w2_common_aux_mix(lane, salt + axis1_w2_common_only);
    axis1_w2_common_counter += (mix & 15) - 3;
    axis1_w2_common_only ^= (lane + salt + axis1_w2_common_counter);
    return mix + axis1_w2_common_counter;
}
