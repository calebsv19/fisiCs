extern const int axis1_w1_interleave_weights[4];
extern int axis1_w1_interleave_bias_seed;
int axis1_w1_interleave_aux_mix(int lane, int seed);

static int op_add(int a, int b) { return a + b; }
static int op_xor(int a, int b) { return (a ^ b) + 3; }
static int op_mul(int a, int b) { return a * (b | 1); }
static int op_mix(int a, int b) { return (a * 7) + (b * 5) + 11; }

typedef int (*AxisFn)(int, int);

static AxisFn k_axis_fns[4] = {op_add, op_xor, op_mul, op_mix};

int axis1_w1_interleave_step(int lane, int seed) {
    int idx = lane & 3;
    int a = seed + axis1_w1_interleave_weights[idx];
    int b = axis1_w1_interleave_aux_mix(lane, seed + axis1_w1_interleave_bias_seed);
    int out = k_axis_fns[idx](a, b);
    axis1_w1_interleave_bias_seed = axis1_w1_interleave_bias_seed * 5 + idx + 1;
    return out ^ axis1_w1_interleave_bias_seed;
}
