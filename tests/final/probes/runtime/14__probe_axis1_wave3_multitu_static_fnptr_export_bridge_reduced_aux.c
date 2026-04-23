extern int (*axis1_wave3_reduced_pick)(int);

int axis1_wave3_reduced_apply(int seed) {
    int first = axis1_wave3_reduced_pick(seed);
    int second = axis1_wave3_reduced_pick(first);
    return second;
}
