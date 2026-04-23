extern int axis1_w2_ws_symbol_a;
extern int axis1_w2_ws_symbol_b;

int axis1_w2_ws_symbol_b = -9;

int axis1_w2_ws_aux_mix(int lane, int salt) {
    int v = salt * (lane + 3) + axis1_w2_ws_symbol_a * 5 - axis1_w2_ws_symbol_b * 2;
    axis1_w2_ws_symbol_b += (lane & 1) ? 2 : 3;
    return v ^ (lane * 13 + 17);
}

int axis1_w2_ws_aux_fold(int lane, int salt) {
    int mix = axis1_w2_ws_aux_mix(lane + 1, salt + axis1_w2_ws_symbol_a);
    axis1_w2_ws_symbol_a ^= (lane * 5 + 3);
    return mix + axis1_w2_ws_symbol_a - axis1_w2_ws_symbol_b;
}
