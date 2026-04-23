typedef int (*axis1_wave5_op_t)(int);

extern const axis1_wave5_op_t axis1_wave5_ops[2];

int axis1_wave5_bridge_eval(int seed) {
    const axis1_wave5_op_t first = axis1_wave5_ops[0];
    const axis1_wave5_op_t second = axis1_wave5_ops[1];
    return second(first(seed)) - first(1);
}
