extern int axis1_w2_common_counter;
extern int axis1_w2_common_only;

int axis1_w2_common_only = 0;

int axis1_w2_common_aux_mix(int lane, int salt) {
    int base = axis1_w2_common_counter * 9 + axis1_w2_common_only * 5 + salt;
    axis1_w2_common_only += (lane & 3) + 1;
    return (base ^ (lane * 17 + 11)) + axis1_w2_common_only;
}

int axis1_w2_common_aux_shift(int lane, int salt) {
    axis1_w2_common_counter ^= (lane * 7 + salt);
    axis1_w2_common_only += (axis1_w2_common_counter & 3);
    return axis1_w2_common_counter - axis1_w2_common_only;
}
