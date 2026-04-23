extern int (*axis1_wave4_dispatch_table[2])(int);

int axis1_wave4_apply(int seed) {
    int first = axis1_wave4_dispatch_table[0](seed);
    int second = axis1_wave4_dispatch_table[1](first);
    return second + axis1_wave4_dispatch_table[0](1);
}
