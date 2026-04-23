const int axis1_w1_interleave_weights[4] = {5, 11, 7, 13};
int axis1_w1_interleave_bias_seed = 9;

int axis1_w1_interleave_aux_mix(int lane, int seed) {
    int idx = lane & 3;
    int v = seed * (idx + 3) + axis1_w1_interleave_weights[(idx + 1) & 3];
    return (v ^ (lane * 17 + 19)) + axis1_w1_interleave_bias_seed;
}
