extern int (*axis1_wave3_pick(int lane))(int);

int axis1_wave3_apply(int seed) {
    int (*first)(int) = axis1_wave3_pick(0);
    int (*second)(int) = axis1_wave3_pick(1);
    return second(first(seed));
}
