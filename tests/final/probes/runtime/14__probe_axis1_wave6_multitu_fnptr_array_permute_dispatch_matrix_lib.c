typedef int (*axis1_wave6_binop_t)(int, int);

static int axis1_wave6_bin_add(int left, int right) {
    return left + right;
}

static int axis1_wave6_bin_xor(int left, int right) {
    return left ^ right;
}

static int axis1_wave6_bin_muladd(int left, int right) {
    return (left * 2) + right;
}

axis1_wave6_binop_t axis1_wave6_dispatch_ops[3] = {
    axis1_wave6_bin_add,
    axis1_wave6_bin_xor,
    axis1_wave6_bin_muladd,
};

const int axis1_wave6_dispatch_perm[3] = {2, 0, 1};
