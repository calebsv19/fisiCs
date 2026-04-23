extern int axis1_w2_ws_symbol_a;
extern int axis1_w2_ws_symbol_b;
extern int axis1_w2_ws_aux_mix(int lane, int salt);

int axis1_w2_ws_symbol_a = 41;

int axis1_w2_ws_step(int lane, int salt) {
    int base = axis1_w2_ws_aux_mix(lane, salt + axis1_w2_ws_symbol_b);
    axis1_w2_ws_symbol_a += (base & 7) + lane;
    axis1_w2_ws_symbol_b -= (salt & 3) - 1;
    return base ^ (axis1_w2_ws_symbol_a * 3 + axis1_w2_ws_symbol_b);
}
