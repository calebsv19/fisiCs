typedef int (*axis1_wave6_binop_t)(int, int);

extern axis1_wave6_binop_t axis1_wave6_dispatch_ops[3];
extern const int axis1_wave6_dispatch_perm[3];

int axis1_wave6_permute_eval(int seed) {
    int acc = seed;
    int lane = 0;
    for (; lane < 3; ++lane) {
        int idx = axis1_wave6_dispatch_perm[lane];
        axis1_wave6_binop_t op = axis1_wave6_dispatch_ops[idx];
        acc = op(acc, lane + 4);
    }
    return acc + axis1_wave6_dispatch_ops[axis1_wave6_dispatch_perm[0]](1, 2);
}
