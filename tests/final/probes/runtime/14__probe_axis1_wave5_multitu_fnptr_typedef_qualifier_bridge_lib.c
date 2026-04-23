typedef int (*axis1_wave5_op_t)(int);

static int axis1_wave5_add2(int value) {
    return value + 2;
}

static int axis1_wave5_mul3(int value) {
    return value * 3;
}

const axis1_wave5_op_t axis1_wave5_ops[2] = {
    axis1_wave5_add2,
    axis1_wave5_mul3,
};
